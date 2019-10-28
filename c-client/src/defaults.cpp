//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <limits>

#include <dwave_sapi.h>

using std::numeric_limits;

namespace {
enum Magic {
  QuantumSolverParams = 0,
  SwSampleParams = 1,
  SwOptimizeParams = 2,
  SwHeuristicParams = 3
};
} // namespace {anonymous}

DWAVE_SAPI const sapi_QuantumSolverParameters SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS = {
    QuantumSolverParams,
    numeric_limits<int>::min(), // annealing_time
    static_cast<sapi_SolverParameterAnswerMode>(numeric_limits<int>::min()), // answer_mode
    numeric_limits<int>::min(), // auto_scale
    -numeric_limits<double>::max(), // beta
    0, // chains
    numeric_limits<int>::min(), // max_answers
    numeric_limits<int>::min(), // num_reads
    numeric_limits<int>::min(), // num_spin_reversal_transforms
    static_cast<sapi_Postprocess>(numeric_limits<int>::min()), // postprocess
    numeric_limits<int>::min(), // programming_thermalization
    numeric_limits<int>::min(), // readout_thermalization
    0, // anneal_offsets
    0, // anneal_schedule
    0, // reverse_anneal
    0, // flux_biases
    numeric_limits<int>::min(), // flux_drift_compensation
    numeric_limits<int>::min() // reduce_intersample_correlation
};

DWAVE_SAPI const sapi_SwSampleSolverParameters SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS = {
    SwSampleParams,
    SAPI_ANSWER_MODE_HISTOGRAM, // answer_mode
    3.0, // beta
    numeric_limits<int>::max(), // max_answers
    1, // num_reads
    0, // use_random_seed
    0 // random_seed
};

DWAVE_SAPI const sapi_SwOptimizeSolverParameters SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS = {
    SwOptimizeParams,
    SAPI_ANSWER_MODE_HISTOGRAM, // answer_mode
    numeric_limits<int>::max(), // max_answers
    1, // num_reads
};

DWAVE_SAPI const sapi_SwHeuristicSolverParameters SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS = {
    SwHeuristicParams,
    10, // iteration_limit
    1.0 / 8.0, // max_bit_flip_prob
    9, // max_local_complexity
    1.0 / 32.0, // min_bit_flip_prob
    8, // local_stuck_limit
    4, // num_perturbed_copies
    0, // num_variables
    0, // use_random_seed
    0, // random_seed
    5.0 // time_limit_seconds
};
