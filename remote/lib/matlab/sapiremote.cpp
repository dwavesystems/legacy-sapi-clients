//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <mex.h>

#include <ctime>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <string>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

#include <exceptions.hpp>

#include "sapiremote-mex.hpp"

using std::bad_alloc;
using std::map;
using std::ostringstream;
using std::shared_ptr;
using std::string;

const char* mexName = "sapiremote_mex";

namespace err_id {
namespace internal {
const char* numArgs = "sapiremote:internal:BadNumArgs";
const char* numOut = "sapiremote:internal:BadNumOutputs";
const char* argType = "sapiremote:internal:BadArgType";
} // namespace err_id::internal

const char* argType = "sapiremote:BadArgType";
const char* badHandle = "sapiremote:InvalidHandle";
const char* badSolver = "sapiremote:UnknownSolver";
const char* badProblem = "sapiremote:BadProblemData";
const char* solveError = "sapiremote:SolveError";
const char* asyncNotDone = "sapiremote:AsyncNotDone";
const char* authError = "sapiremote:AuthenticationFailed";
const char* networkError = "sapiremote:NetworkError";

const char* error = "sapiremote:Error";
} // namespace err_id

namespace subcommands {
const auto done = "done";
const auto answer = "answer";
const auto awaitSubmission = "awaitsubmission";
const auto awaitCompletion = "awaitcompletion";
const auto cancel = "cancel";
const auto retry = "retry";
const auto problemId = "problemid";
const auto status = "status";
const auto solvers = "solvers";
const auto encodeQp = "encodeqp";
const auto submitProblem = "submitproblem";
const auto addProblem = "addproblem";
const auto decodeQp = "decodeqp";
} // namespace subcommands

namespace {

typedef void (* Subfunction)(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]);
typedef map<string, Subfunction> DispatchMap;

DispatchMap initDispatchMap() {
  DispatchMap dm;
  dm[subcommands::answer] = subfunctions::answer;
  dm[subcommands::done] = subfunctions::done;
  dm[subcommands::awaitSubmission] = subfunctions::awaitSubmission;
  dm[subcommands::awaitCompletion] = subfunctions::awaitCompletion;
  dm[subcommands::cancel] = subfunctions::cancel;
  dm[subcommands::retry] = subfunctions::retry;
  dm[subcommands::problemId] = subfunctions::problemId;
  dm[subcommands::status] = subfunctions::status;
  dm[subcommands::solvers] = subfunctions::solvers;
  dm[subcommands::encodeQp] = subfunctions::encodeQp;
  dm[subcommands::submitProblem] = subfunctions::submitProblem;
  dm[subcommands::addProblem] = subfunctions::addProblem;
  dm[subcommands::decodeQp] = subfunctions::decodeQp;
  return dm;
}

const DispatchMap& dispatchMap() {
  static DispatchMap dm = initDispatchMap();
  return dm;
}

void commandDispatch(const mxArray* cmdArray, int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  if (mxGetClassID(cmdArray) != mxCHAR_CLASS) {
    mexErrMsgIdAndTxt("sapi:internal:BadSubcommandType", "Subcommand must be a char array");
  }

  const DispatchMap& dm = dispatchMap();
  shared_ptr<char> cmdString(mxArrayToString(cmdArray), mxFree);
  DispatchMap::const_iterator cmdIter = dm.find(cmdString.get());
  if (cmdIter == dm.end()) {
    mexErrMsgIdAndTxt("sapi:internal:BadSubcommand", "Unknown subcommand");
  }

  cmdIter->second(nlhs, plhs, nrhs, prhs);
}

void init() {
  static bool done = false;
  if (!done) {
    mexAtExit(exitFn);
    done = true;
  }
}

} // namespace {anonymous}

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  init();
  if (nrhs == 0) mexErrMsgIdAndTxt(err_id::internal::numArgs, "No command given");
  try {
    commandDispatch(prhs[0], nlhs, plhs, nrhs - 1, prhs + 1);
  } catch (sapiremote::AuthenticationException& e) {
    mexErrMsgIdAndTxt(err_id::authError, "%s", e.what());
  } catch (sapiremote::NetworkException& e) {
    mexErrMsgIdAndTxt(err_id::networkError, "%s", e.what());
  } catch (sapiremote::NoAnswerException& e) {
    mexErrMsgIdAndTxt(err_id::asyncNotDone, "%s", e.what());
  } catch (sapiremote::EncodingException& e) {
    mexErrMsgIdAndTxt(err_id::badProblem, "%s", e.what());
  } catch (sapiremote::SolveException& e) {
    mexErrMsgIdAndTxt(err_id::solveError, "%s", e.what());
  } catch (sapiremote::Exception& e) {
    mexErrMsgIdAndTxt(err_id::error, "%s", e.what());
  } catch (std::bad_alloc&) {
    mexErrMsgTxt("Out of memory");
  }
}

string uniqueId() {
  static const std::time_t t = std::time(0);
  static const std::clock_t c = std::clock();
  static int n = 0;
  ostringstream id;
  id << std::hex << t << c << n++;
  return id.str();
}

void failBadHandle() {
  mexErrMsgIdAndTxt(err_id::badHandle, "Invalid handle");
}
