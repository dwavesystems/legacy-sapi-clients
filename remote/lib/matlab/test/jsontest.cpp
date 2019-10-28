//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <string>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/variant/apply_visitor.hpp>

#include <mex.h>

#include <json.hpp>

#include "../mxfreedeleter.hpp"
#include "../json-to-matlab.hpp"

using std::string;
using std::unique_ptr;
using std::vector;

struct UnambiguousJsonVisitor : boost::static_visitor<mxArray*> {
  mxArray* operator()(json::Null) const { return mxCreateDoubleMatrix(0, 0, mxREAL); }
  mxArray* operator()(bool x) const { return mxCreateLogicalScalar(x); }
  mxArray* operator()(double x) const { return mxCreateDoubleScalar(x); }
  mxArray* operator()(json::Integer x) const { return mxCreateDoubleScalar(x); }
  mxArray* operator()(const string& x) const { return mxCreateString(x.c_str()); }

  mxArray* operator()(const json::Array& x) const {
    auto size = x.size();
    auto arrArray = mxCreateCellMatrix(size, 1);
    for (auto i = 0u; i < size; ++i) {
      mxSetCell(arrArray, i, boost::apply_visitor(*this, x[i].variant()));
    }
    return arrArray;
  }

  mxArray* operator()(const json::Object& x) const {
    auto size = x.size();
    auto objArray = mxCreateCellMatrix(size, 2);
    auto i = 0u;
    BOOST_FOREACH( const auto& v, x ) {
      mxSetCell(objArray, i, mxCreateString(v.first.c_str()));
      mxSetCell(objArray, i + size, boost::apply_visitor(*this, v.second.variant()));
      ++i;
    }
    return objArray;
  }
};

mxArray* unambiguousMatlabJson(const mxArray* jsonArray) {
  auto jv = matlabToJson(jsonArray);
  return boost::apply_visitor(UnambiguousJsonVisitor(), jv.variant());
}

mxArray* stringToMatlabJson(const mxArray* stringArray) {
  auto s = unique_ptr<char, MxFreeDeleter>(mxArrayToString(stringArray));
  if (!s) mexErrMsgIdAndTxt("jsontest:parse:type", "'parse' command requires a string argument");
  return jsonToMatlab(json::stringToJson(s.get()));
}


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 2) mexErrMsgIdAndTxt("jsontest:numArgs", "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt("jsontest:numOut", "Too many outputs");

  auto cmd = unique_ptr<char, MxFreeDeleter>(mxArrayToString(prhs[0]));
  if (!cmd) mexErrMsgIdAndTxt("jsontest:type", "First argument must be a string");
  auto cmdString = string(cmd.get());

  if (cmdString == "parse") {
    plhs[0] = stringToMatlabJson(prhs[1]);
  } else if (cmdString == "unambiguous") {
    plhs[0] = unambiguousMatlabJson(prhs[1]);
  } else {
    mexErrMsgIdAndTxt("jsontest:badCommand", "Command string must be 'parse' or 'unambiguous'");
  }
}
