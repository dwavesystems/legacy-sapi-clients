//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <problem.hpp>
#include <coding.hpp>

#include <dwave_sapi.h>
#include <sapi-impl.hpp>
#include <internal.hpp>
#include <remote.hpp>

using std::current_exception;
using std::numeric_limits;
using std::sort;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::vector;

using sapi::makeProblemManager;
using sapi::handleException;
using sapi::IsingResultPtr;
using sapi::InvalidParameterException;
using sapi::ConnectionPtr;
using sapi::RemoteConnection;
using sapi::SolverPtr;
using sapi::RemoteSolver;
using sapi::SolverMap;


namespace {

namespace propkeys {
const auto supportedProblemTypes = "supported_problem_types";
const auto numQubits = "num_qubits";
const auto qubits = "qubits";
const auto couplers = "couplers";
const auto hRange = "h_range";
const auto jRange = "j_range";
const auto annealOffsetRanges = "anneal_offset_ranges";
const auto annealOffsetStep = "anneal_offset_step";
const auto annealOffsetStepPhi0 = "anneal_offset_step_phi0";
const auto maxAnnealSchedulePoints = "max_anneal_schedule_points";
const auto annealingTimeRange = "annealing_time_range";
const auto parameters = "parameters";
const auto extendedJRange = "extended_j_range";
const auto perQubitCouplingRange = "per_qubit_coupling_range";
} // namespace propkeys

namespace paramkeys {
const auto annealingTime = "annealing_time";
const auto answerMode = "answer_mode";
const auto autoScale = "auto_scale";
const auto beta = "beta";
const auto chains = "chains";
const auto maxAnswers = "max_answers";
const auto numReads = "num_reads";
const auto numSpinReversalTransforms = "num_spin_reversal_transforms";
const auto postprocess = "postprocess";
const auto programmingThermalization = "programming_thermalization";
const auto readoutThermalization = "readout_thermalization";
const auto annealOffsets = "anneal_offsets";
const auto annealSchedule = "anneal_schedule";
const auto initialState = "initial_state";
const auto reinitializeState = "reinitialize_state";
const auto fluxBiases = "flux_biases";
const auto fluxDriftCompensation = "flux_drift_compensation";
const auto reduceIntersampleCorrelation = "reduce_intersample_correlation";
} // namespace paramkeys

namespace timingkeys {
const auto timing = "timing";
const auto runTimeChip = "run_time_chip";
const auto annealTime = "anneal_time_per_run";
const auto readoutTime = "readout_time_per_run";
const auto totalRealTime = "total_real_time";

const auto qpuAccessTime = "qpu_access_time";
const auto qpuProgramming_time = "qpu_programming_time";
const auto qpuSamplingTime = "qpu_sampling_time";
const auto qpuAnnealTimePerSample = "qpu_anneal_time_per_sample";
const auto qpuReadoutTimePerSample = "qpu_readout_time_per_sample";
const auto qpuDelayTimePerSample = "qpu_delay_time_per_sample";

const auto totalPostprocessingTime = "total_post_processing_time";
const auto postProcessingOverhead = "post_processing_overhead_time";
} // namespace {anonymous}::timingkeys

const auto defaultTiming = sapi_Timing{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};


long long extractTimingVal(const json::Object timingInfo, const char* key) {
  auto it = timingInfo.find(key);
  if (it != timingInfo.end() && it->second.isInteger()) {
    return it->second.getInteger();
  } else {
    return -1;
  }
}


sapi_Timing extractTiming(const json::Object& answer) {
  auto timing = defaultTiming;
  auto it = answer.find(timingkeys::timing);
  if (it != answer.end() && it->second.isObject()) {
    const auto& timingObj = it->second.getObject();
    timing.qpu_access_time = extractTimingVal(timingObj, timingkeys::qpuAccessTime);
    timing.qpu_programming_time = extractTimingVal(timingObj, timingkeys::qpuProgramming_time);
    timing.qpu_sampling_time = extractTimingVal(timingObj, timingkeys::qpuSamplingTime);
    timing.qpu_anneal_time_per_sample = extractTimingVal(timingObj, timingkeys::qpuAnnealTimePerSample);
    timing.qpu_readout_time_per_sample = extractTimingVal(timingObj, timingkeys::qpuReadoutTimePerSample);
    timing.qpu_delay_time_per_sample = extractTimingVal(timingObj, timingkeys::qpuDelayTimePerSample);
    timing.total_post_processing_time = extractTimingVal(timingObj, timingkeys::totalPostprocessingTime);
    timing.post_processing_overhead_time = extractTimingVal(timingObj, timingkeys::postProcessingOverhead);

    // deprecated values -- populate from new values if necessary
    timing.run_time_chip = extractTimingVal(timingObj, timingkeys::runTimeChip);
    timing.anneal_time_per_run = extractTimingVal(timingObj, timingkeys::annealTime);
    timing.readout_time_per_run = extractTimingVal(timingObj, timingkeys::readoutTime);
    timing.total_real_time = extractTimingVal(timingObj, timingkeys::totalRealTime);

    if (timing.run_time_chip == -1) timing.run_time_chip = timing.qpu_sampling_time;
    if (timing.anneal_time_per_run == -1) timing.anneal_time_per_run = timing.qpu_anneal_time_per_sample;
    if (timing.readout_time_per_run == -1) timing.readout_time_per_run = timing.qpu_readout_time_per_sample;
    if (timing.total_real_time == -1) timing.total_real_time = timing.qpu_access_time;
  }
  return timing;
}


const char* decodeProblemType(int t) {
  switch (t) {
    case SAPI_PROBLEM_TYPE_ISING: return "ising";
    case SAPI_PROBLEM_TYPE_QUBO: return "qubo";
    default: throw InvalidParameterException("invalid problem type");
  }
}

const char* answerModeStr(int m) {
  switch (m) {
    case SAPI_ANSWER_MODE_HISTOGRAM: return "histogram";
    case SAPI_ANSWER_MODE_RAW: return "raw";
    default: throw InvalidParameterException("invalid answer_mode parameter");
  }
}

const char* postprocessStr(sapi_Postprocess p) {
  switch (p) {
    case SAPI_POSTPROCESS_NONE: return "";
    case SAPI_POSTPROCESS_OPTIMIZATION: return "optimization";
    case SAPI_POSTPROCESS_SAMPLING: return "sampling";
    default: throw InvalidParameterException("invalid postprocess parameter");
  }
}

json::Value encodeProblem(sapiremote::SolverPtr solver, const sapi_Problem* p) {
  auto qpep = reinterpret_cast<const sapiremote::QpProblemEntry*>(p->elements);
  auto rp = sapiremote::QpProblem(qpep, qpep + p->len);
  return sapiremote::encodeQpProblem(solver, std::move(rp));
}

SolverMap remoteSolverMap(const sapiremote::ProblemManagerPtr& pm) {
  auto solverMap = SolverMap{};
  BOOST_FOREACH( const auto& e, pm->fetchSolvers() ) {
    solverMap[e.first] = SolverPtr(new RemoteSolver(e.second));
  }
  return solverMap;
};

} // namespace {anonymous}

namespace sapi {

IsingResultPtr decodeRemoteIsingResult(const tuple<string, json::Value>& result) {
  if (sapiremote::answerFormat(std::get<1>(result)) != sapiremote::FORMAT_QP)
    throw UnsupportedAnswerFormatException("unsupported answer format");

  const json::Object& resultObject = std::get<1>(result).getObject();

  sapiremote::QpAnswer answer = sapiremote::decodeQpAnswer(std::get<0>(result), resultObject);

  auto ret = IsingResultPtr(new sapi_IsingResult);
  ret->energies = 0;
  ret->num_occurrences = 0;
  ret->solutions = 0;
  ret->num_solutions = static_cast<int>(answer.energies.size());
  ret->solution_len = answer.energies.empty() ? 0 : answer.solutions.size() / answer.energies.size();

  ret->energies = new double[answer.energies.size()];
  std::copy(answer.energies.begin(), answer.energies.end(), ret->energies);

  if (!answer.numOccurrences.empty()) {
    ret->num_occurrences = new int[answer.numOccurrences.size()];
    std::copy(answer.numOccurrences.begin(), answer.numOccurrences.end(), ret->num_occurrences);
  }

  ret->solutions = new int[answer.solutions.size()];
  std::copy(answer.solutions.begin(), answer.solutions.end(), ret->solutions);

  ret->timing = extractTiming(resultObject);
  return ret;
}


json::Object quantumParametersToJson(const sapi_QuantumSolverParameters& params) {
  auto ret = json::Object();
  const auto& d = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;

  if (params.annealing_time != d.annealing_time) ret[paramkeys::annealingTime] = params.annealing_time;
  if (params.answer_mode != d.answer_mode) ret[paramkeys::answerMode] = answerModeStr(params.answer_mode);
  if (params.auto_scale != d.auto_scale) ret[paramkeys::autoScale] = params.auto_scale != 0;
  if (params.beta != d.beta) ret[paramkeys::beta] = params.beta;
  if (params.max_answers != d.max_answers) ret[paramkeys::maxAnswers] = params.max_answers;
  if (params.num_reads != d.num_reads) ret[paramkeys::numReads] = params.num_reads;
  if (params.num_spin_reversal_transforms != d.num_spin_reversal_transforms) {
    ret[paramkeys::numSpinReversalTransforms] = params.num_spin_reversal_transforms;
  }
  if (params.postprocess != d.postprocess) ret[paramkeys::postprocess] = postprocessStr(params.postprocess);
  if (params.programming_thermalization != d.programming_thermalization) {
    ret[paramkeys::programmingThermalization] = params.programming_thermalization;
  }
  if (params.readout_thermalization != d.readout_thermalization) {
    ret[paramkeys::readoutThermalization] = params.readout_thermalization;
  }

  if (params.chains != d.chains) {
    auto jsonChains = json::Array();
    unordered_map<int, int> chainIndex;
    auto nextChainIndex = 0;
    for (size_t i = 0; i < params.chains->len; ++i) {
      auto ci = params.chains->elements[i];
      if (ci < 0) continue;
      auto iter = chainIndex.find(ci);
      if (iter == chainIndex.end()) {
        chainIndex[ci] = nextChainIndex;
        ++nextChainIndex;
        jsonChains.push_back(json::Array{static_cast<json::Integer>(i)});
      } else {
        jsonChains[iter->second].getArray().push_back(static_cast<json::Integer>(i));
      }
    }
    ret[paramkeys::chains] = std::move(jsonChains);
  }

  if (params.anneal_offsets != d.anneal_offsets) {
    const auto& ao = *params.anneal_offsets;
    ret[paramkeys::annealOffsets] = json::Array(ao.elements, ao.elements + ao.len);
  }

  if (params.anneal_schedule != d.anneal_schedule) {
    const auto& as = *params.anneal_schedule;
    auto jsonSched = json::Array{};
    jsonSched.reserve(as.len);
    for (size_t i = 0; i < as.len; ++i) {
      jsonSched.push_back(json::Array{as.elements[i].time, as.elements[i].relative_current});
    }
    ret[paramkeys::annealSchedule] = std::move(jsonSched);
  }

  if (params.reverse_anneal != d.reverse_anneal) {
    const auto& re = *params.reverse_anneal;
    auto jsonInitState = json::Array{};
    jsonInitState.reserve(re.initial_state_len);
    for (size_t i = 0; i < re.initial_state_len; ++i) {
      jsonInitState.push_back(re.initial_state[i]);
    }
    ret[paramkeys::initialState] = std::move(jsonInitState);
    ret[paramkeys::reinitializeState] = re.reinitialize_state != 0;
  }

  if (params.flux_biases != d.flux_biases) {
    const auto& fb = *params.flux_biases;
    ret[paramkeys::fluxBiases] = json::Array(fb.elements, fb.elements + fb.len);
  }

  if (params.flux_drift_compensation != d.flux_drift_compensation) {
    ret[paramkeys::fluxDriftCompensation] = params.flux_drift_compensation != 0;
  }
  if (params.reduce_intersample_correlation != d.reduce_intersample_correlation) {
    ret[paramkeys::reduceIntersampleCorrelation] = params.reduce_intersample_correlation != 0;
  }

  return ret;
}


RemoteSupportedProblemTypesProperty::RemoteSupportedProblemTypesProperty(const json::Object& jsonProps)
    : valid_(false) {
  auto iter = jsonProps.find(propkeys::supportedProblemTypes);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto jsonSpt = iter->second.getArray();
  BOOST_FOREACH( const auto& spt, jsonSpt ) {
    if (spt.isString()) probTypes_.push_back(spt.getString());
  }
  BOOST_FOREACH( const auto& spt, probTypes_ ) {
    probTypesCStr_.push_back(spt.c_str());
  }
  prop_.elements = probTypesCStr_.data();
  prop_.len = probTypesCStr_.size();
  valid_ = true;
}


RemoteQuantumSolverProperties::RemoteQuantumSolverProperties(const json::Object& jsonProps) : valid_(false) {
  if (jsonProps.find(propkeys::numQubits) == jsonProps.end()
      || jsonProps.find(propkeys::qubits) == jsonProps.end()
      || jsonProps.find(propkeys::couplers) == jsonProps.end()) return;

  const auto imin = numeric_limits<int>::min();
  const auto imax = numeric_limits<int>::max();

  // num_qubits
  auto iter = jsonProps.find(propkeys::numQubits);
  if (!iter->second.isInteger()) return;
  auto numQubits = iter->second.getInteger();
  if (numQubits < imin || numQubits > imax) return;
  props_.num_qubits = static_cast<int>(numQubits);

  // qubits
  vector<int> qubits;
  iter = jsonProps.find(propkeys::qubits);
  if (!iter->second.isArray()) return;
  const auto& jsonQubits = iter->second.getArray();
  BOOST_FOREACH( const auto& q, jsonQubits ) {
    if (!q.isInteger()) return;
    auto qi = q.getInteger();
    if (qi < imin || qi > imax) return;
    qubits.push_back(static_cast<int>(qi));
  }

  // couplers
  vector<sapi_Coupler> couplers;
  iter = jsonProps.find(propkeys::couplers);
  if (!iter->second.isArray()) return;
  const auto& jsonCouplers = iter->second.getArray();
  BOOST_FOREACH( const auto& c, jsonCouplers ) {
    if (!c.isArray()) return;
    const auto& ca = c.getArray();
    if (ca.size() != 2) return;
    if (!ca[0].isInteger() || !ca[1].isInteger()) return;
    auto q0 = ca[0].getInteger();
    auto q1 = ca[1].getInteger();
    if (q0 < imin || q0 > imax || q1 < imin || q1 > imax) return;
    couplers.push_back(sapi_Coupler{static_cast<int>(q0), static_cast<int>(q1)});
  }

  // commit
  qubits_ = std::move(qubits);
  couplers_ = std::move(couplers);
  props_.qubits_len = qubits_.size();
  props_.qubits = qubits_.data();
  props_.couplers_len = couplers_.size();
  props_.couplers = couplers_.data();
  valid_ = true;
}


RemoteIsingRangeProperties::RemoteIsingRangeProperties(const json::Object& jsonProps) : valid_(false) {
  // h range
  auto iter = jsonProps.find(propkeys::hRange);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto& jsonHRange = iter->second.getArray();
  if (jsonHRange.size() != 2 || !jsonHRange[0].isReal() || !jsonHRange[1].isReal()) return;
  props_.h_min = jsonHRange[0].getReal();
  props_.h_max = jsonHRange[1].getReal();

  // J range
  iter = jsonProps.find(propkeys::jRange);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto& jsonJRange = iter->second.getArray();
  if (jsonJRange.size() != 2 || !jsonJRange[0].isReal() || !jsonJRange[1].isReal()) return;
  props_.j_min = jsonJRange[0].getReal();
  props_.j_max = jsonJRange[1].getReal();

  valid_ = true;
}


RemoteAnnealOffsetProperties::RemoteAnnealOffsetProperties(const json::Object& jsonProps) : valid_(false) {
  // anneal_offset_ranges
  auto iter = jsonProps.find(propkeys::annealOffsetRanges);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto& jsonRanges = iter->second.getArray();
  vector<sapi_AnnealOffsetRange> ranges;
  ranges.reserve(jsonRanges.size());
  BOOST_FOREACH( const auto& r, jsonRanges ) {
    if (!r.isArray()) return;
    const auto& jsonRange = r.getArray();
    if (jsonRange.size() != 2) return;
    if (!jsonRange[0].isReal() || !jsonRange[1].isReal()) return;
    ranges.push_back(sapi_AnnealOffsetRange{jsonRange[0].getReal(), jsonRange[1].getReal()});
  }

  // anneal_offset_step
  props_.step = -1.0;
  iter = jsonProps.find(propkeys::annealOffsetStep);
  if (iter != jsonProps.end() && iter->second.isReal()) props_.step = iter->second.getReal();

  // anneal_offset_step_phi0
  props_.step_phi0 = -1.0;
  iter = jsonProps.find(propkeys::annealOffsetStepPhi0);
  if (iter != jsonProps.end() && iter->second.isReal()) props_.step_phi0 = iter->second.getReal();

  // commit
  ranges_ = std::move(ranges);
  props_.ranges_len = ranges_.size();
  props_.ranges = ranges_.data();
  valid_ = true;
}


RemoteAnnealScheduleProperties::RemoteAnnealScheduleProperties(const json::Object& jsonProps) : valid_(false) {
  props_.max_points = -1;
  props_.min_annealing_time = -1.0;
  props_.max_annealing_time = -1.0;

  auto hasMaxPoints = false;
  auto iter = jsonProps.find(propkeys::maxAnnealSchedulePoints);
  if (iter != jsonProps.end() && iter->second.isInteger()) {
    props_.max_points = static_cast<int>(iter->second.getInteger());
    hasMaxPoints = true;
  }

  auto hasRange = false;
  iter = jsonProps.find(propkeys::annealingTimeRange);
  if (iter != jsonProps.end() && iter->second.isArray()) {
    const auto& rangeArray = iter->second.getArray();
    if (rangeArray.size() == 2 && rangeArray[0].isReal() && rangeArray[1].isReal()) {
      props_.min_annealing_time = rangeArray[0].getReal();
      props_.max_annealing_time = rangeArray[1].getReal();
      hasRange = true;
    }
  }

  valid_ = hasMaxPoints || hasRange;
}


RemoteParametersProperty::RemoteParametersProperty(const json::Object& jsonProps) : valid_(false) {
  auto iter = jsonProps.find(propkeys::parameters);
  if (iter == jsonProps.end() || !iter->second.isObject()) return;
  const auto& jsonParams = iter->second.getObject();

  BOOST_FOREACH( const auto& e, jsonParams ) {
    params_.push_back(e.first);
  }
  sort(params_.begin(), params_.end());
  paramsCStr_.reserve(params_.size());
  BOOST_FOREACH( const auto& s, params_ ) {
    paramsCStr_.push_back(s.c_str());
  }
  prop_.elements = paramsCStr_.data();
  prop_.len = paramsCStr_.size();
  valid_ = true;
}


RemoteVirtualGraphProperties::RemoteVirtualGraphProperties(const json::Object& jsonProps) : valid_(false) {
  // extended_j_range
  auto iter = jsonProps.find(propkeys::extendedJRange);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto& jsonEJR = iter->second.getArray();
  if (jsonEJR.size() != 2 || !jsonEJR[0].isReal() || !jsonEJR[1].isReal()) return;
  props_.extended_j_min = jsonEJR[0].getReal();
  props_.extended_j_max = jsonEJR[1].getReal();

  // per_qubit_coupling_range
  iter = jsonProps.find(propkeys::perQubitCouplingRange);
  if (iter == jsonProps.end() || !iter->second.isArray()) return;
  const auto& jsonPQCR = iter->second.getArray();
  if (jsonPQCR.size() != 2 || !jsonPQCR[0].isReal() || !jsonPQCR[1].isReal()) return;
  props_.per_qubit_coupling_min = jsonPQCR[0].getReal();
  props_.per_qubit_coupling_max = jsonPQCR[1].getReal();

  valid_ = true;
}


void RemoteParameterValidator::operator()(const json::Object& params) const {
  if (hasValidParamNames_) {
    BOOST_FOREACH( const auto& e, params ) {
      if (validParamNames_.find(e.first) == validParamNames_.end()) {
        throw InvalidParameterException("invalid parameter for this solver: " + e.first);
      }
    }
  }
}

RemoteParameterValidator::RemoteParameterValidator(const json::Object& solverProps) : hasValidParamNames_(false) {
  auto sptIter = solverProps.find(propkeys::parameters);
  if (sptIter != solverProps.end() && sptIter->second.isObject()) {
    const auto& sptObj = sptIter->second.getObject();
    BOOST_FOREACH( const auto& e, sptObj ) {
      validParamNames_.insert(e.first);
    }
    hasValidParamNames_ = true;
  }
}


IsingResultPtr RemoteSolver::solveImpl(
    sapi_ProblemType type, const sapi_Problem *problem, const sapi_SolverParameters *params) const {

  auto sp = submit(type, problem, params);
  while (!sapiremote::awaitCompletion({sp->remoteSubmittedProblem()}, 1, 3600.0));
  return sp->result();
}


SubmittedProblemPtr RemoteSolver::submitImpl(
    sapi_ProblemType type, const sapi_Problem *problem, const sapi_SolverParameters *params) const {

  if (params->parameter_unique_id != SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS.parameter_unique_id) {
    throw InvalidParameterException("remote solvers require sapi_QuantumSolverParameters parameters argument");
  }

  auto rtype = decodeProblemType(type);
  auto rproblem = encodeProblem(rsolver_, problem);
  auto rparams = quantumParametersToJson(*reinterpret_cast<const sapi_QuantumSolverParameters*>(params));
  validateParamNames_(rparams);
  return SubmittedProblemPtr(new RemoteSubmittedProblem(rsolver_->submitProblem(rtype, rproblem, rparams)));
}


RemoteConnection::RemoteConnection(const sapiremote::ProblemManagerPtr& problemManager) :
    sapi_Connection(remoteSolverMap(problemManager)),
    problemManager_(problemManager) {}

} // namespace sapi

DWAVE_SAPI sapi_Code sapi_remoteConnection(
    const char* url,
    const char* token,
    const char* proxy_url,
    sapi_Connection** connOut,
    char* err_msg) {

  try {
    auto pm = makeProblemManager(url, token, proxy_url);
    auto conn = ConnectionPtr(new RemoteConnection(pm));
    *connOut = conn.release();
    return SAPI_OK;
  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}
