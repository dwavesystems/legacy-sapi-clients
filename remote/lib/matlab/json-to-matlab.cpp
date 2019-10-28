//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>
#include <cstddef>
#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/variant.hpp>

#include <json.hpp>

#include "mxfreedeleter.hpp"
#include "json-to-matlab.hpp"

using std::size_t;
using std::make_pair;
using std::map;
using std::numeric_limits;
using std::reverse;
using std::set;
using std::stack;
using std::string;
using std::transform;
using std::unique_ptr;
using std::vector;

namespace {

size_t initMaxFieldSize() {
  mxArray* result;
  mexCallMATLAB(1, &result, 0, 0, "namelengthmax");
  return static_cast<size_t>(mxGetScalar(result));
}

size_t maxFieldSize() {
  static size_t mfs = initMaxFieldSize();
  return mfs;
}

string sanitizeFieldName(string f) {
  if (f.empty() || !isalpha(f[0])) {
    f.insert(0, "X");
  }
  BOOST_FOREACH( char& c, f ) {
    if (!isalnum(c)) c = '_';
  }
  size_t mfs = maxFieldSize();
  if (f.size() > mfs) {
    f.erase(f.begin() + mfs, f.end());
  }
  return f;
}

class ArrayDimVisitor : public boost::static_visitor<bool> {
private:
  bool consistent_;
  bool numeric_;
  bool reachedBottom_;
  unsigned level_;
  vector<mwSize> dims_;

  class NextLevel {
  private:
    unsigned& level_;
  public:
    NextLevel(unsigned& level) : level_(level) { ++level_; }
    ~NextLevel() { --level_; }
  };

public:
  ArrayDimVisitor() : consistent_(true), numeric_(true), reachedBottom_(false), level_(0) {}

  bool consistent() const { return consistent_; }
  bool numeric() const { return numeric_; }
  const vector<mwSize> dims() const { return dims_; }

  bool operator()(const json::Array& arr) {
    if (level_ >= dims_.size()) {
      if (reachedBottom_) return consistent_ = false;
      dims_.push_back(arr.size());
    } else if (dims_[level_] != arr.size()) {
      return consistent_ = false;
    }

    if (arr.empty()) reachedBottom_ = true;
    NextLevel l(level_);

    BOOST_FOREACH( const auto& v, arr) {
      if (!v.variant().apply_visitor(*this)) return false;
    }
    return true;
  }

  bool operator()(bool) {
    if (reachedBottom_ && numeric_) return consistent_ = false;
    reachedBottom_ = true;
    numeric_ = false;
    return true;
  }

  bool operator()(json::Integer) {
    if (reachedBottom_ && !numeric_) return consistent_ = false;
    reachedBottom_ = true;
    numeric_ = true;
    return true;
  }

  bool operator()(double) {
    if (reachedBottom_ && !numeric_) return consistent_ = false;
    reachedBottom_ = true;
    numeric_ = true;
    return true;
  }

  template<typename T>
  bool operator()(const T&) {
    return consistent_ = false;
  }
};

template<typename D>
D convertValue(const json::Value& v);

template<>
double convertValue<double>(const json::Value& v) {
  return v.getReal();
}

template<>
mxLogical convertValue<mxLogical>(const json::Value& v) {
  return v.getBool();
}

template<typename D>
void fillArray(const json::Array& jsonArray, D*& data) {
  if (jsonArray.empty()) return;

  if (jsonArray.front().isArray()) {
    BOOST_FOREACH( const auto& v, jsonArray ) {
      fillArray<D>(v.getArray(), data);
    }
  } else {
    transform(jsonArray.begin(), jsonArray.end(), data, convertValue<D>);
    data += jsonArray.size();
  }
}

class JsonToMatlabVisitor : public boost::static_visitor<mxArray*> {
public:
  mxArray* operator()(const json::Null&) const { return mxCreateDoubleMatrix(0, 0, mxREAL); }
  mxArray* operator()(bool b) const { return mxCreateLogicalScalar(b); }
  mxArray* operator()(json::Integer n) const { return mxCreateDoubleScalar(static_cast<double>(n)); }
  mxArray* operator()(double x) const { return mxCreateDoubleScalar(x); }
  mxArray* operator()(const string& s) const { return mxCreateString(s.c_str()); }

  mxArray* operator()(const json::Array& arr) const {
    ArrayDimVisitor visit;

    if (visit(arr)) {
      auto dims = visit.dims();
      reverse(dims.begin(), dims.end());
      if (visit.numeric()) {
        auto resultArray = mxCreateNumericArray(dims.size(), &dims[0], mxDOUBLE_CLASS, mxREAL);
        auto data = mxGetPr(resultArray);
        fillArray<double>(arr, data);
        return resultArray;
      } else {
        auto resultArray = mxCreateLogicalArray(dims.size(), &dims[0]);
        auto data = static_cast<mxLogical*>(mxGetData(resultArray));
        fillArray<mxLogical>(arr, data);
        return resultArray;
      }

    } else {
      auto n = arr.size();
      auto resultArray = mxCreateCellMatrix(1, n);
      for (auto i = 0u; i < n; ++i) {
        mxSetCell(resultArray, i, jsonToMatlab(arr[i]));
      }
      return resultArray;
    }
  }

  mxArray* operator()(const json::Object& obj) const {
    if (obj.empty()) return mxCreateStructMatrix(1, 1, 0, 0);

    vector<string> sanitizedFieldNames;
    sanitizedFieldNames.reserve(obj.size());
    vector<const char*> cstrFieldNames;
    cstrFieldNames.reserve(obj.size());

    BOOST_FOREACH( const auto& v, obj ) {
      sanitizedFieldNames.push_back(sanitizeFieldName(v.first));
      cstrFieldNames.push_back(sanitizedFieldNames.back().c_str());
    }

    mxArray* objectArray = mxCreateStructMatrix(1, 1, static_cast<int>(cstrFieldNames.size()),
      &cstrFieldNames.front());
    auto i = 0;
    BOOST_FOREACH( const auto& v, obj ) {
      mxSetFieldByNumber(objectArray, 0, i, jsonToMatlab(v.second));
      ++i;
    }
    return objectArray;
  }
};

vector<mwSize> arrayDims(const mxArray* arr) {
  auto dims = mxGetDimensions(arr);
  auto numDims = mxGetNumberOfDimensions(arr);
  vector<mwSize> dimSizes;
  dimSizes.reserve(numDims);
  BOOST_FOREACH( auto d, make_pair(dims, dims + numDims) ) {
    if (d > 1) dimSizes.push_back(d);
  }
  return dimSizes;
}

template<typename T>
json::Value sparseArrayToJson(const mxArray* arr) {
  auto m = mxGetM(arr);
  auto n = mxGetN(arr);
  auto ir = mxGetIr(arr);
  auto jc = mxGetJc(arr);
  auto pr = static_cast<const T*>(mxGetData(arr));

  auto ret = json::Array(n, json::Array(m, T()));
  for (auto ci = 0 * n; ci < n; ++ci) {
    auto& retc = ret[ci].getArray();
    for (auto ri = jc[ci]; ri < jc[ci + 1]; ++ri) {
      retc[ir[ri]] = pr[ri];
    }
  }

  return ret;
}

template<typename T>
json::Value numericArrayToJson(const mxArray* arr) {
  if (mxIsEmpty(arr)) return json::Array();
  if (mxIsSparse(arr)) return sparseArrayToJson<T>(arr);
  if (mxGetNumberOfElements(arr) == 1) return *static_cast<const T*>(mxGetData(arr));

  auto dimSizes = arrayDims(arr);
  vector<json::Array> jsonArrays(dimSizes.size());
  auto& innerArray = jsonArrays.front();
  auto data = static_cast<const T*>(mxGetData(arr));
  auto numDims = dimSizes.size();
  BOOST_FOREACH ( auto p, make_pair(data, data + mxGetNumberOfElements(arr))) {
    innerArray.push_back(p);
    for (auto i = 0 * numDims; i < numDims - 1; ++i) {
      if (jsonArrays[i].size() == dimSizes[i]) {
        jsonArrays[i + 1].push_back(std::move(jsonArrays[i]));
      }
    }
  }
  return std::move(jsonArrays.back());
}

json::Value structToJson(const mxArray* arr, int arrIndex) {
  auto numFields = mxGetNumberOfFields(arr);
  json::Object obj;
  for (auto i = 0; i < numFields; ++i) {
    obj[mxGetFieldNameByNumber(arr, i)] = matlabToJson(mxGetFieldByNumber(arr, arrIndex, i));
  }
  return obj;
}

json::Value structArrayToJson(const mxArray* arr) {
  if (mxIsEmpty(arr)) return json::Object();
  if (mxGetNumberOfElements(arr) == 1) return structToJson(arr, 0);

  auto dimSizes = arrayDims(arr);
  vector<json::Array> jsonArrays(dimSizes.size());
  auto& innerArray = jsonArrays.front();
  auto numDims = dimSizes.size();
  auto numElems = mxGetNumberOfElements(arr);
  for (auto i = 0u; i < numElems; ++i) {
    innerArray.push_back(structToJson(arr, i));
    for (auto d = 0u; d < numDims - 1; ++d) {
      if (jsonArrays[d].size() == dimSizes[d]) {
        jsonArrays[d + 1].push_back(std::move(jsonArrays[d]));
      }
    }
  }
  return std::move(jsonArrays.back());
}

json::Value cellArrayToJson(const mxArray* arr) {
  if (mxIsEmpty(arr)) return json::Array();
  if (mxGetNumberOfElements(arr) == 1) return json::Array{matlabToJson(mxGetCell(arr, 0))};

  auto dimSizes = arrayDims(arr);
  vector<json::Array> jsonArrays(dimSizes.size());
  auto& innerArray = jsonArrays.front();
  auto numDims = dimSizes.size();
  auto numElems = mxGetNumberOfElements(arr);
  for (auto i = 0u; i < numElems; ++i) {
    innerArray.push_back(matlabToJson(mxGetCell(arr, i)));
    for (auto d = 0u; d < numDims - 1; ++d) {
      if (jsonArrays[d].size() == dimSizes[d]) {
        jsonArrays[d + 1].push_back(std::move(jsonArrays[d]));
      }
    }
  }
  return std::move(jsonArrays.back());
}

} // namespace {anonymous}


mxArray* jsonToMatlab(const json::Value& v) {
  return boost::apply_visitor(JsonToMatlabVisitor(), v.variant());
}


json::Value matlabToJson(const mxArray* arr) {
  switch (mxGetClassID(arr)) {
    case mxCELL_CLASS:
      return cellArrayToJson(arr);

    case mxCHAR_CLASS:
      return string(unique_ptr<char, MxFreeDeleter>(mxArrayToString(arr)).get());

    case mxSTRUCT_CLASS:
      return structArrayToJson(arr);

    case mxLOGICAL_CLASS:
      return numericArrayToJson<mxLogical>(arr);

    case mxDOUBLE_CLASS:
      return numericArrayToJson<double>(arr);

    case mxSINGLE_CLASS:
      return numericArrayToJson<float>(arr);

    case mxINT8_CLASS:
      return numericArrayToJson<int8_T>(arr);

    case mxUINT8_CLASS:
      return numericArrayToJson<uint8_T>(arr);

    case mxINT16_CLASS:
      return numericArrayToJson<int16_T>(arr);

    case mxUINT16_CLASS:
      return numericArrayToJson<uint16_T>(arr);

    case mxINT32_CLASS:
      return numericArrayToJson<int32_T>(arr);

    case mxUINT32_CLASS:
      return numericArrayToJson<uint32_T>(arr);

    case mxINT64_CLASS:
      return numericArrayToJson<int64_T>(arr);

    case mxUINT64_CLASS:
      return numericArrayToJson<uint64_T>(arr);

    default:
      //mxUNKNOWN_CLASS
      //mxFUNCTION_CLASS
      //mxVOID_CLASS
      throw json::ValueException();
  }
}
