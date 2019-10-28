//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cmath>
#include <cstring>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <mex.h>

#include <boost/numeric/conversion/cast.hpp>

#include <types.hpp>
#include <problem.hpp>
#include <coding.hpp>

#include "sapiremote-mex.hpp"
#include "mxfreedeleter.hpp"

using std::clock;
using std::ceil;
using std::strcmp;
using std::copy;
using std::get;
using std::numeric_limits;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;

using boost::numeric_cast;

using sapiremote::SubmittedProblemPtr;
using sapiremote::QpProblem;
using sapiremote::QpProblemEntry;
using sapiremote::answerFormat;
using sapiremote::encodeQpProblem;
using sapiremote::decodeQpAnswer;
using sapiremote::toString;

namespace submittedstates = sapiremote::submittedstates;
namespace remotestatuses = sapiremote::remotestatuses;
namespace errortypes = sapiremote::errortypes;

namespace {

typedef unordered_map<uint64_T, SubmittedProblemPtr> SubmittedProblemMap;

namespace fields {
auto problemId = "problem_id";
auto internalId = "internal_id";
auto connection = "connection";
auto remoteStatus = "remote_status";
auto state = "state";
auto lastGoodState = "last_good_state";
auto errorType = "error_type";
auto errorMessage = "error_message";
auto submittedOn = "submitted_on";
auto solvedOn = "solved_on";
} // namespace {anonymous}::fields

namespace qpfields {
const auto energies = "energies";
const auto solutions = "solutions";
const auto numOccurrences = "num_occurrences";
} // namespace {anonymous}::qpfields

namespace answerkeys {
const auto format = "format";
const auto activeVars = "active_variables";
const auto numVars = "num_variables";
const auto solutions = "solutions";
const auto energies = "energies";
const auto numOccurrences = "num_occurrences";
} // namespace {anonymous}::answerkeys

const char* spFields[] = { fields::internalId, fields::connection };
const auto numSpFields = sizeof(spFields) / sizeof(spFields[0]);

const char* statusFields[] = { fields::problemId, fields::state, fields::remoteStatus };
const auto numStatusFields = sizeof(statusFields) / sizeof(statusFields[0]);

SubmittedProblemMap& submittedProblems() {
  static SubmittedProblemMap spMap;
  return spMap;
}

uint64_T initInternalId() {
  auto ns = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
  auto c = clock();
  auto x = static_cast<uint64_T>(c) ^ static_cast<uint64_T>(ns);
  return x ^ (x >> 13) ^ (x << 46);
}

uint64_T nextInternalId() {
  static uint64_T id = initInternalId();
  id = 6364136223846793005ull * id + 1442695040888963407ull;
  return id;
}

mxArray* nextInternalIdArray() {
  auto array = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  *static_cast<uint64_T*>(mxGetData(array)) = nextInternalId();
  return array;
}

uint64_T extractInternalId(const mxArray* array) {
  auto iiArray = mxGetField(array, 0, fields::internalId);
  if (!iiArray) failBadHandle();
  if (!mxIsUint64(iiArray)) failBadHandle();
  return *static_cast<uint64_T*>(mxGetData(iiArray));
}

string extractProblemId(const mxArray* array) {
  auto piArray = mxGetField(array, 0, fields::problemId);
  if (!piArray) failBadHandle();
  auto idp = unique_ptr<char, MxFreeDeleter>(mxArrayToString(piArray));
  if (!idp) failBadHandle();
  return string(idp.get());
}

ConnectionPtr extractConnection(const mxArray* array) {
  auto cArray = mxGetField(array, 0, fields::connection);
  if (!cArray) failBadHandle();
  return getConnection(cArray);
}

SubmittedProblemPtr getSubmittedProblem(const mxArray* spArray, bool remove) {
  auto internalId = extractInternalId(spArray);
  auto iter = submittedProblems().find(internalId);
  if (iter != submittedProblems().end()) {
    auto sp = iter->second;
    if (remove) submittedProblems().erase(iter);
    return sp;
  }

  auto sp = extractConnection(spArray)->problemManager()->addProblem(extractProblemId(spArray));
  if (!remove) submittedProblems()[internalId] = sp;
  return sp;
}

mxArray* createSubmittedProblemArray(SubmittedProblemPtr sp, const mxArray* connArray) {
  auto id = nextInternalId();
  auto idArray = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
  *static_cast<uint64_T*>(mxGetData(idArray)) = id;

  auto spArray = mxCreateStructMatrix(1, 1, numSpFields, spFields);
  mxSetField(spArray, 0, fields::internalId, idArray);
  mxSetField(spArray, 0, fields::connection, mxDuplicateArray(connArray));

  submittedProblems()[id] = sp;

  return spArray;
}

mxArray* addProblemId(SubmittedProblemPtr sp, const mxArray* handle) {
  auto newHandle = mxDuplicateArray(handle);
  auto nf = mxGetNumberOfFields(handle);
  for (auto i = 0; i < nf; ++i) {
    if (strcmp(mxGetFieldNameByNumber(newHandle, i), fields::problemId) == 0) {
      mxDestroyArray(mxGetFieldByNumber(newHandle, 0, i));
      mxSetFieldByNumber(newHandle, 0, i, mxCreateString(sp->problemId().c_str()));
      return newHandle;
    }
  }

  auto i = mxAddField(newHandle, fields::problemId);
  if (i == -1) throw std::bad_alloc();
  mxSetFieldByNumber(newHandle, 0, i, mxCreateString(sp->problemId().c_str()));
  return newHandle;
}

mxArray* qpToMatlab(const string& problemType, json::Object answer) {
  auto qpAnswer = decodeQpAnswer(problemType, answer);
  auto hasNumOcc = answer.find(answerkeys::numOccurrences) != answer.end();
  answer.erase(answerkeys::format);
  answer.erase(answerkeys::activeVars);
  answer.erase(answerkeys::numVars);
  answer.erase(answerkeys::solutions);
  answer.erase(answerkeys::energies);
  answer.erase(answerkeys::numOccurrences);

  auto answerArr = jsonToMatlab(answer);

  auto numSols = qpAnswer.energies.size();
  auto solSize = numSols > 0 ? qpAnswer.solutions.size() / numSols : 0;

  auto energiesArr = mxCreateDoubleMatrix(1, numSols, mxREAL);
  copy(qpAnswer.energies.begin(), qpAnswer.energies.end(), mxGetPr(energiesArr));
  auto i = mxAddField(answerArr, qpfields::energies);
  if (i == -1) throw std::bad_alloc();
  mxSetFieldByNumber(answerArr, 0, i, energiesArr);

  auto solutionsArr = mxCreateDoubleMatrix(solSize, numSols, mxREAL);
  copy(qpAnswer.solutions.begin(), qpAnswer.solutions.end(), mxGetPr(solutionsArr));
  i = mxAddField(answerArr, qpfields::solutions);
  if (i == -1) throw std::bad_alloc();
  mxSetFieldByNumber(answerArr, 0, i, solutionsArr);

  if (hasNumOcc) {
    auto numOccArr = mxCreateDoubleMatrix(1, numSols, mxREAL);
    copy(qpAnswer.numOccurrences.begin(), qpAnswer.numOccurrences.end(), mxGetPr(numOccArr));
    auto i = mxAddField(answerArr, qpfields::numOccurrences);
    if (i == -1) throw std::bad_alloc();
    mxSetFieldByNumber(answerArr, 0, i, numOccArr);
  }

  return answerArr;
}

QpProblem matlabToQpProblem(const mxArray* arr) {
  if (!mxIsDouble(arr) || mxIsComplex(arr) || mxGetNumberOfDimensions(arr) != 2) {
    mexErrMsgIdAndTxt(err_id::badProblem, "Problem must be a real 2-dimensional matrix of doubles");
  }

  QpProblem problem;

  try {
    if (mxIsSparse(arr)) {
      auto ir = mxGetIr(arr);
      auto jc = mxGetJc(arr);
      auto data = mxGetPr(arr);
      auto n = numeric_cast<int>(mxGetN(arr));
      for (auto jci = 0; jci < n; ++jci) {
        for (auto i = jc[jci]; i < jc[jci + 1]; ++i) {
          problem.push_back(QpProblemEntry{numeric_cast<int>(ir[i]), jci, data[i]});
        }
      }

    } else {
      auto m = numeric_cast<int>(mxGetM(arr));
      auto n = numeric_cast<int>(mxGetN(arr));
      auto data = mxGetPr(arr);

      for (auto j = 0; j < n; ++j) {
        for (auto i = 0; i < m; ++i) {
          if (*data != 0) problem.push_back(QpProblemEntry{i, j, *data});
          ++data;
        }
      }
    }

  } catch (boost::bad_numeric_cast&) {
    mexErrMsgIdAndTxt(err_id::badProblem, "Problem dimensions too large");
  }

  return problem;
}

} // namespace {anonymous}

namespace subfunctions {

void answer(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto answer = getSubmittedProblem(prhs[0], true)->answer();
  plhs[0] = mxCreateCellMatrix(2, 1);
  mxSetCell(plhs[0], 0, mxCreateString(std::get<0>(answer).c_str()));
  mxSetCell(plhs[0], 1, jsonToMatlab(std::get<1>(answer)));
}

void awaitSubmission(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 2) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  if (!mxIsDouble(prhs[1]) || mxGetNumberOfElements(prhs[1]) != 1) {
    mexErrMsgIdAndTxt(err_id::argType, "Invalid timeout");
  }
  auto timeout = mxGetScalar(prhs[1]);

  if (!mxIsCell(prhs[0])) mexErrMsgIdAndTxt(err_id::badHandle, "Expected cell array");
  vector<SubmittedProblemPtr> problems;
  const auto n = mxGetNumberOfElements(prhs[0]);
  for (size_t i = 0; i < n; ++i) {
    problems.push_back(getSubmittedProblem(mxGetCell(prhs[0], i), false));
  }
  sapiremote::awaitSubmission(problems, timeout);

  plhs[0] = mxCreateCellMatrix(1, n);
  for (size_t i = 0; i < n; ++i) {
    if (problems[i]->problemId().empty()) {
      mxSetCell(plhs[0], i, mxCreateDoubleMatrix(0, 0, mxREAL));
    } else {
      mxSetCell(plhs[0], i, addProblemId(problems[i], mxGetCell(prhs[0], i)));
    }
  }
}

void awaitCompletion(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 3) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  if (!mxIsDouble(prhs[2]) || mxGetNumberOfElements(prhs[2]) != 1) {
    mexErrMsgIdAndTxt(err_id::argType, "Invalid timeout");
  }
  auto timeout = mxGetScalar(prhs[2]);

  auto minDoneArg = ceil(mxGetScalar(prhs[1]));
  size_t minDone;
  if (minDoneArg >= 0 && minDoneArg <= numeric_limits<size_t>::max()) {
    minDone = static_cast<size_t>(minDoneArg);
  } else if (minDoneArg > 0) {
    minDone = numeric_limits<size_t>::max();
  } else {
    minDone = 0;
  }

  if (!mxIsCell(prhs[0])) mexErrMsgIdAndTxt(err_id::badHandle, "Expected cell array");
  vector<SubmittedProblemPtr> problems;
  const auto n = mxGetNumberOfElements(prhs[0]);
  for (size_t i = 0; i < n; ++i) {
    problems.push_back(getSubmittedProblem(mxGetCell(prhs[0], i), false));
  }

  sapiremote::awaitCompletion(problems, minDone, timeout);

  plhs[0] = mxCreateLogicalMatrix(1, n);
  auto data = mxGetLogicals(plhs[0]);
  for (size_t i = 0; i < n; ++i) {
    data[i] = static_cast<mxLogical>(problems[i]->done());
  }
}

void done(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  plhs[0] = mxCreateLogicalScalar(getSubmittedProblem(prhs[0], false)->done());
}

void cancel(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 0) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  getSubmittedProblem(prhs[0], false)->cancel();
}

void retry(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 0) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  getSubmittedProblem(prhs[0], false)->retry();
}

void problemId(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  plhs[0] = mxCreateString(getSubmittedProblem(prhs[0], false)->problemId().c_str());
}

void status(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 1) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto status = getSubmittedProblem(prhs[0], false)->status();
  plhs[0] = mxCreateStructMatrix(1, 1, numStatusFields, statusFields);
  mxSetField(plhs[0], 0, fields::problemId, mxCreateString(status.problemId.c_str()));
  mxSetField(plhs[0], 0, fields::remoteStatus, mxCreateString(toString(status.remoteStatus).c_str()));
  mxSetField(plhs[0], 0, fields::state, mxCreateString(toString(status.state).c_str()));

  if (status.state == submittedstates::RETRYING || status.state == submittedstates::FAILED) {
    auto r = mxAddField(plhs[0], fields::lastGoodState);
    if (r == -1) throw std::bad_alloc();
    mxSetField(plhs[0], 0, fields::lastGoodState, mxCreateString(toString(status.lastGoodState).c_str()));
  }

  if (status.state == submittedstates::RETRYING || status.state == submittedstates::FAILED
      || (status.state == submittedstates::DONE && status.remoteStatus != remotestatuses::COMPLETED)) {
    auto r = mxAddField(plhs[0], fields::errorType);
    if (r == -1) throw std::bad_alloc();
    mxSetField(plhs[0], 0, fields::errorType, mxCreateString(toString(status.error.type).c_str()));
    r = mxAddField(plhs[0], fields::errorMessage);
    if (r == -1) throw std::bad_alloc();
    mxSetField(plhs[0], 0, fields::errorMessage, mxCreateString(status.error.message.c_str()));
  }

  if (!status.submittedOn.empty()) {
    auto r = mxAddField(plhs[0], fields::submittedOn);
    if (r == -1) throw std::bad_alloc();
    mxSetFieldByNumber(plhs[0], 0, r, mxCreateString(status.submittedOn.c_str()));
  }

  if (!status.solvedOn.empty()) {
    auto r = mxAddField(plhs[0], fields::solvedOn);
    if (r == -1) throw std::bad_alloc();
    mxSetFieldByNumber(plhs[0], 0, r, mxCreateString(status.solvedOn.c_str()));
  }
}

void encodeQp(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 2) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto solver = getSolver(prhs[0]);
  auto problem = matlabToQpProblem(prhs[1]);
  plhs[0] = jsonToMatlab(encodeQpProblem(solver, problem));
}

void submitProblem(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 4) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto connArray = mxGetField(prhs[0], 0, fields::connection);
  if (!connArray) failBadHandle();

  auto solver = getSolver(prhs[0]);
  auto type = unique_ptr<char, MxFreeDeleter>(mxArrayToString(prhs[1]));
  if (!type) mexErrMsgIdAndTxt(err_id::argType, "Problem type must be a string");
  json::Value problem;
  try {
    problem = matlabToJson(prhs[2]);
  } catch (json::Exception&) {
    mexErrMsgIdAndTxt(err_id::badProblem, "Unable to convert problem to JSON");
  }
  json::Object params;
  try {
    params = matlabToJson(prhs[3]).getObject();
  } catch (json::Exception&) {
    mexErrMsgIdAndTxt(err_id::badProblem, "Unable to convert parameters to JSON object");
  }

  plhs[0] = createSubmittedProblemArray(solver->submitProblem(type.get(), problem, params), connArray);
}

void addProblem(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 2) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  auto conn = getConnection(prhs[0]);
  auto id = unique_ptr<char, MxFreeDeleter>(mxArrayToString(prhs[1]));
  if (!id) mexErrMsgIdAndTxt(err_id::argType, "Problem ID must be a string");

  plhs[0] = createSubmittedProblemArray(conn->problemManager()->addProblem(id.get()), prhs[0]);
}

void decodeQp(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (nrhs != 2) mexErrMsgIdAndTxt(err_id::internal::numArgs, "Wrong number of arguments");
  if (nlhs > 1) mexErrMsgIdAndTxt(err_id::internal::numOut, "Wrong number of outputs");

  if (!mxIsChar(prhs[0])) mexErrMsgIdAndTxt(err_id::argType, "Problem type must be a string");
  auto type = unique_ptr<char, MxFreeDeleter>(mxArrayToString(prhs[0]));
  if (!type) throw std::bad_alloc();

  if (!mxIsStruct(prhs[1])) mexErrMsgIdAndTxt(err_id::argType, "Answer must be a struct");
  auto answer = matlabToJson(prhs[1]);

  plhs[0] = qpToMatlab(type.get(), std::move(answer.getObject()));
}

} // namespace subfunctions
