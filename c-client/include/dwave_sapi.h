//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef DWAVE_SAPI_H_INCLUDED
#define DWAVE_SAPI_H_INCLUDED

/*
Proprietary Information D-Wave Systems Inc.
Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
Notice this code is licensed to authorized users only under the
applicable license agreement see eula.txt
D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
*/

/*! \file dwave_sapi.h
\brief A D-Wave SAPI header file.
D-Wave SAPI C Interface
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef DWAVE_SAPI_BUILD
    #define DWAVE_SAPI __declspec(dllexport)
  #else
    #define DWAVE_SAPI __declspec(dllimport)
  #endif
#else
  #define DWAVE_SAPI
#endif /* _WIN32 */

/* macro */

/**
* \brief sapi error message maximum size
*/
#define SAPI_ERROR_MESSAGE_MAX_SIZE 512


/* enum */

/**
* \brief sapi error code enum.
*
* SAPI_OK: function call is successful, no error occurred.
* SAPI_ERR_INVALID_PARAMETER: invalid parameter.
* SAPI_ERR_SOLVE_FAILED: error occurred during the solve.
* SAPI_ERR_AUTHENTICATION: authentication failed.
* SAPI_ERR_NETWORK: network error occurred.
* SAPI_ERR_COMMUNICATION: network communication failed.
* SAPI_ERR_ASYNC_NOT_DONE: problem not done.
* SAPI_ERR_PROBLEM_CANCELLED: problem cancelled.
* SAPI_ERR_NO_INIT: sapi_globalInit not called.
* SAPI_ERR_OUT_OF_MEMORY: no enough memory.
* SAPI_ERR_NO_EMBEDDING_FOUND: no solution available or exist.
*/
typedef enum sapi_Code
{
  SAPI_OK = 0,
  SAPI_ERR_INVALID_PARAMETER,
  SAPI_ERR_SOLVE_FAILED,
  SAPI_ERR_AUTHENTICATION,
  SAPI_ERR_NETWORK,
  SAPI_ERR_COMMUNICATION,
  SAPI_ERR_ASYNC_NOT_DONE,
  SAPI_ERR_PROBLEM_CANCELLED,
  SAPI_ERR_NO_INIT,
  SAPI_ERR_OUT_OF_MEMORY,
  SAPI_ERR_NO_EMBEDDING_FOUND
} sapi_Code;


/**
* \brief sapi solver answer mode parameter enum.
*
* SAPI_ANSWER_MODE_HISTOGRAM: histogram mode.
* SAPI_ANSWER_MODE_RAW: raw mode.
*/
typedef enum sapi_SolverParameterAnswerMode
{
  SAPI_ANSWER_MODE_HISTOGRAM = 0,
  SAPI_ANSWER_MODE_RAW
} sapi_SolverParameterAnswerMode;


/**
* \brief sapi fix variables method enum.
*
* SAPI_FIX_VARIABLES_METHOD_OPTIMIZED: optimized.
* SAPI_FIX_VARIABLES_METHOD_STANDARD: standard.
*/
/**
* \brief sapi fix variables solver parameters
*
*  determines the algorithm used to infer values.
*  SAPI_FIX_VARIABLES_METHOD_OPTIMIZED: uses roof-duality & strongly connected components
*  SAPI_FIX_VARIABLES_METHOD_STANDARD: uses roof-duality only
*
*  fix variables algorithm uses maximum flow in the implication network to correctly
*  fix variables (that is, one can find an assignment for the other variables that
*  attains the optimal value). The variables that roof duality fixes will take the
*  same values in all optimal solutions.
*
*  Using strongly connected components can fix more variables, but in some optimal
*  solutions these variables may take different values.
*
*  In summary:
*    * All the variables fixed by SAPI_FIX_VARIABLES_METHOD_STANDARD will also be fixed by
*      SAPI_FIX_VARIABLES_METHOD_OPTIMIZED (reverse is not true)
*    * All the variables fixed by SAPI_FIX_VARIABLES_METHOD_STANDARD will take the same value in
*      every optimal solution
*    * There exists at least one optimal solution that has the fixed values as given
*      by SAPI_FIX_VARIABLES_METHOD_OPTIMIZED
*
*  Thus, SAPI_FIX_VARIABLES_METHOD_STANDARD is a subset of SAPI_FIX_VARIABLES_METHOD_OPTIMIZED as any
*  variable that is fixed by SAPI_FIX_VARIABLES_METHOD_STANDARD will also be fixed by
*  SAPI_FIX_VARIABLES_METHOD_OPTIMIZED and additionally, SAPI_FIX_VARIABLES_METHOD_OPTIMIZED may fix
*  some variables that SAPI_FIX_VARIABLES_METHOD_STANDARD could not. For this reason,
*  SAPI_FIX_VARIABLES_METHOD_OPTIMIZED takes longer than SAPI_FIX_VARIABLES_METHOD_STANDARD.
*/
typedef enum sapi_FixVariablesMethod
{
  SAPI_FIX_VARIABLES_METHOD_OPTIMIZED = 0,
  SAPI_FIX_VARIABLES_METHOD_STANDARD
} sapi_FixVariablesMethod;

/**
* \brief sapi problem type enum.
*
* SAPI_PROBLEM_TYPE_ISING: ising problem.
* SAPI_PROBLEM_TYPE_QUBO: qubo problem.
*/
typedef enum sapi_ProblemType
{
  SAPI_PROBLEM_TYPE_ISING = 0,
  SAPI_PROBLEM_TYPE_QUBO
} sapi_ProblemType;

/*
 * Broken chain repair strategies.  Used by sapi_unembedAnswer.
 *
 * SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY: greedy descent on original problem.
 *   Intact chains aren't changed.
 * SAPI_BROKEN_CHAINS_VOTE: use majority value of each chain.
 *   In case of tie, choose -1/1 with equal probability.
 * SAPI_BROKEN_CHAINS_DISCARD: discard any answer with broken chains.
 * SAPI_BROKEN_CHAINS_WEIGHTED_RANDOM: choose value randomly,
 *   with P(+1) = number of +1 in chain / chain size
 */
typedef enum sapi_BrokenChains
{
  SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY,
  SAPI_BROKEN_CHAINS_VOTE,
  SAPI_BROKEN_CHAINS_DISCARD,
  SAPI_BROKEN_CHAINS_WEIGHTED_RANDOM
} sapi_BrokenChains;

/* function pointer */

/**
* \brief sapi qsage objective function signature.
*
* \param states can contain -1/1 (ising) or 0/1 (qubo).
* \param len the length of the states array.
* \param num_states number of states. Note that each state's length is len / num_states.
* \param extra_arg user can pass extra argument.
* \param result its length should be num_states.
*\ return sapi error code.
*/
typedef sapi_Code (*sapi_QSageObjectiveFunction)(const int* states, size_t len, size_t num_states, void* extra_arg, double* result);

/**
* \brief sapi qsage lp solver function signature, a solver that can solve linear programming
*        problems.
*
*  Finds the minimum of a problem specified by
*    min      f * x
*    st.      Aineq * x <= bineq
*               Aeq * x = beq
*               lb <= x <= ub
*
* \param f linear objective function, its length is num_vars.
* \param Aineq linear inequality constraints, its length is Aineq_size * num_vars.
* \param bineq righthand side for linear inequality constraints, its length is Aineq_size.
* \param Aeq linear equality constraints, its length is Aeq_size * num_vars.
* \param beq righthand side for linear equality constraints, its length is Aeq_size.
* \param lb lower bounds, its length is num_vars.
* \param ub upper bounds, its length is num_vars.
* \param num_vars number of variables.
* \param Aineq_size the length of Aineq.
* \param Aeq_size the length of Aeq.
* \param extra_arg user can pass extra argument.
* \param result its length should be num_vars.
* \return sapi error code
*/
typedef sapi_Code (*sapi_QSageLPSolver)(const double* f, const double* Aineq, const double* bineq, const double* Aeq, const double* beq,
                                                                           const double* lb, const double* ub, size_t num_vars, size_t Aineq_size, size_t Aeq_size, void* extra_arg, double* result);


/* parameter */

/**
* \brief Array of doubles with size
*/
typedef struct sapi_DoubleArray
{
  double* elements;
  size_t len;
} sapi_DoubleArray;

/* keep old name for backwards compatibility */
typedef sapi_DoubleArray sapi_AnnealOffsets;

/**
* \brief general sapi solver parameters
*
* parameter_unique_id is the unique id for sapi_SolverParameters structure.
*                     Trying to modify it will cause undefined behaviour.
*/
typedef struct sapi_SolverParameters
{
  const int parameter_unique_id;
} sapi_SolverParameters;

/**
* \brief sapi postprocess enum.
*
* SAPI_POSTPROCESS_NONE: no postprocess (default)
* SAPI_POSTPROCESS_SAMPLING: sampling
* SAPI_POSTPROCESS_OPTIMIZATION: optimization
*/
typedef enum sapi_Postprocess
{
  SAPI_POSTPROCESS_NONE = 0,
  SAPI_POSTPROCESS_SAMPLING,
  SAPI_POSTPROCESS_OPTIMIZATION
} sapi_Postprocess;

typedef enum sapi_RemoteStatus
{
  SAPI_STATUS_UNKNOWN,
  SAPI_STATUS_PENDING,
  SAPI_STATUS_IN_PROGRESS,
  SAPI_STATUS_COMPLETED,
  SAPI_STATUS_FAILED,
  SAPI_STATUS_CANCELED
} sapi_RemoteStatus;

typedef enum sapi_SubmittedState
{
  SAPI_STATE_SUBMITTING,
  SAPI_STATE_SUBMITTED,
  SAPI_STATE_DONE,
  SAPI_STATE_RETRYING,
  SAPI_STATE_FAILED
} sapi_SubmittedState;

/* Embeddings structure
 *
 * elements: mapping from vertices to variables.  That is, each variable
 *   v is represented by vertices i such that elements[i] == v.  Unused
 *   vertices are indicated by elements[i] == -1.
 * len: length of the elements array
 */
typedef struct sapi_Embeddings
{
  int* elements;
  size_t len;
} sapi_Embeddings;

/**
* \brief sapi chains struct.
*
* sapi_Chains is a typedef of sapi_Embeddings for backwards compatibility
*/
typedef struct sapi_Embeddings sapi_Chains;

/**
* \brief A point in an annealing schedule.
*
* time is the time in microseconds
* releative_current is the relative current in the range [0, 1]
*/
typedef struct sapi_AnnealSchedulePoint
{
  double time;
  double relative_current;
} sapi_AnnealSchedulePoint;

/**
* \brief Annealing schedule structure
*
* elements is an array of annealing schedule points. The annealing schedule is the piecewise-linear function
*   connecting these points.
* lin is the length of the elements array.
*/
typedef struct sapi_AnnealSchedule
{
  sapi_AnnealSchedulePoint* elements;
  size_t len;
} sapi_AnnealSchedule;

/**
* \brief Reverse annealing structure
*
* initial_state is the initial state for reverse annealing.  Its format is the same as the solutions field
*   of sapi_IsingResult: {-1, 1} (for Ising problems) or {0, 1} (for QUBO problems) values or 3 for
*   "don't care."
* initial_state_len is the length of initial_state.
* reinitialize_state is a flag indicating whether to reinitialize each read with the initial state (!=0)
*   or use the previous read (==0)
*/
typedef struct sapi_ReverseAnneal
{
  int* initial_state;
  size_t initial_state_len;
  int reinitialize_state;
} sapi_ReverseAnneal;

/**
* \brief sapi quantum solver parameters
*
* parameter_unique_id is the unique id for sapi_QuantumSolverParameters structure.
*                     Trying to modify it will cause undefined behaviour.
* annealing_time a positive integer that sets the duration (in microseconds) of quantum
*                annealing time. (must be an integer > 0, default is hardware specific)
* answer_mode indicates whether to return a histogram of answers, sorted in order of energy
*             (SAPI_ANSWER_MODE_HISTOGRAM); or to return all answers individually in the order they were
*             read (SAPI_ANSWER_MODE_RAW). (must be SAPI_ANSWER_MODE_HISTOGRAM or SAPI_ANSWER_MODE_RAW,
*             default = SAPI_ANSWER_MODE_HISTOGRAM)
* auto_scale indicates whether h and J values will be rescaled to use as much of the range
*            of h and the range of J as possible, or be used as is. When enabled, h and J
*            values need not lie within the range of h and the range of J (but must still
*            be finite). (must be an integer [0 1], default = 1)
* beta Boltzmann distribution parameter. (must be > 0.0, default is hardware specific)
* chains sapi_Embeddings struct (default = NULL)
* max_answers maximum number of answers returned from the solver in histogram mode (which
*             sorts the returned states in order of increasing energy); this is the total
*             number of distinct answers. In raw mode, this limits the returned values to
*             the first max_answers of num_reads samples. Thus, in this mode, max_answers
*             should never be more than num_reads. (must be an integer > 0, default = num_reads)
* num_reads a positive integer that indicates the number of states (output solutions) to
*           read from the solver in each programming cycle. When a hardware solver is used,
*           a programming cycle programs the solver to the specified h and J values for a
*           given ising problem (or Q values for a given qubo problem). However, since
*           hardware precision is limited, the h and J values (or Q values) realized on the
*           solver will deviate slightly from the requested values. On each programming cycle,
*           the deviation is random. (must be an integer > 0, default = 1)
* num_spin_reversal_transforms number of spin reversal transforms. (must be an integer [0 1], default = 0)
* postprocess an enum indicating the kind of postprocess. (SAPI_POSTPROCESS_NONE or SAPI_POSTPROCESS_SAMPLING
*             or SAPI_POSTPROCESS_OPTIMIZATION, default = SAPI_POSTPROCESS_NONE)
* programming_thermalization an integer that gives the time (in microseconds) to wait after
*                            programming the processor in order for it to cool back to base
*                            temperature (i.e., post-programming thermalization time). Lower
*                            values will speed up solving at the expense of solution quality.
*                            (must be an integer > 0, default is hardware specific)
* readout_thermalization an integer that gives the time (in microseconds) to wait after each
*                        state is read from the processor in order for it to cool back to base
*                        temperature (i.e., post-readout thermalization time). Lower values
*                        will speed up solving at the expense of solution quality.
*                        (must be an integer > 0, default is hardware specific)
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +------------------------------+-------------------------------+-----------------------------+
*    |            Field             |               Range           |        Default value        |
*    +==============================+===============================+=============================+
*    |       annealing_time         |                > 0            |       hardware specific     |
*    +------------------------------+-------------------------------+-----------------------------+
*    |         answer_mode          | SAPI_ANSWER_MODE_HISTOGRAM or | SAPI_ANSWER_MODE_HISTOGRAM  |
*    |                              |     SAPI_ANSWER_MODE_RAW      |                             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |         auto_scale           |               [0 1]           |               1             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |           beta               |                > 0.0          |       hardware specific     |
*    +------------------------------+-------------------------------+-----------------------------+
*    |          chains              |                N/A            |             NULL            |
*    +------------------------------+-------------------------------+-----------------------------+
*    |        max_answers           |                > 0            |            num_reads        |
*    +------------------------------+-------------------------------+-----------------------------+
*    |         num_reads            |                > 0            |               1             |
*    +------------------------------+-------------------------------+-----------------------------+
*    | num_spin_reversal_transforms |               [0 1]           |               0             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |                              |  SAPI_POSTPROCESS_NONE   or   |                             |
*    |        postprocess           | SAPI_POSTPROCESS_SAMPLING or  |   SAPI_POSTPROCESS_NONE     |
*    |                              | SAPI_POSTPROCESS_OPTIMIZATION |                             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |  programming_thermalization  |                > 0            |       hardware specific     |
*    +------------------------------+-------------------------------+-----------------------------+
*    |    readout_thermalization    |                > 0            |       hardware specific     |
*    +------------------------------+-------------------------------+-----------------------------+
*/
typedef struct sapi_QuantumSolverParameters
{
  const int parameter_unique_id;
  int annealing_time;
  sapi_SolverParameterAnswerMode answer_mode;
  int auto_scale;
  double beta;
  const sapi_Embeddings* chains;
  int max_answers;
  int num_reads;
  int num_spin_reversal_transforms;
  sapi_Postprocess postprocess;
  int programming_thermalization;
  int readout_thermalization;
  const sapi_DoubleArray* anneal_offsets;
  const sapi_AnnealSchedule* anneal_schedule;
  const sapi_ReverseAnneal* reverse_anneal;
  const sapi_DoubleArray* flux_biases;
  int flux_drift_compensation;
  int reduce_intersample_correlation;
} sapi_QuantumSolverParameters;

/**
* \brief sapi_QuantumSolverParameters default value.
*/
DWAVE_SAPI extern const sapi_QuantumSolverParameters SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;

/**
* \brief sapi software solver (sample) parameters
*
* parameter_unique_id is the unique id for sapi_SwSampleSolverParameters structure.
*                     Trying to modify it will cause undefined behaviour.
* answer_mode indicates whether to return a histogram of answers, sorted in order of energy
*             (SAPI_ANSWER_MODE_HISTOGRAM); or to return all answers individually in the order they were
*             read (SAPI_ANSWER_MODE_RAW). (must be SAPI_ANSWER_MODE_HISTOGRAM or SAPI_ANSWER_MODE_RAW,
*             default = SAPI_ANSWER_MODE_HISTOGRAM)
* beta Boltzmann distribution parameter. The unnormalized probablity of a sample is
*      proportional to exp(-beta * E) where E is its energy. (must be a number >= 0.0, default = 3.0)
* max_answers maximum number of answers returned from the solver in histogram mode (which
*             sorts the returned states in order of increasing energy); this is the total
*             number of distinct answers. In raw mode, this limits the returned values to
*             the first max_answers of num_reads samples. Thus, in this mode, max_answers
*             should never be more than num_reads. (must be an integer > 0, default = num_reads)
* num_reads a positive integer that indicates the number of states (output solutions) to
*           read from the solver in each programming cycle. (must be an integer > 0, default = 1)
* use_random_seed indicates whether to use the random_seed field or not.
*                 (must be an integer [0 1], default = 0)
* random_seed random number generator seed. When a value is provided, solving the same
*             problem with the same parameters will produce the same results every
*             time. If no value is provided, a time-based seed is selected.
*             (must be an integer >= 0, default is a time-based seed)
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +------------------------------+-------------------------------+-----------------------------+
*    |            Field             |               Range           |         Default value       |
*    +==============================+===============================+=============================+
*    |         answer_mode          | SAPI_ANSWER_MODE_HISTOGRAM or | SAPI_ANSWER_MODE_HISTOGRAM  |
*    |                              |     SAPI_ANSWER_MODE_RAW      |                             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |             beta             |                > 0.0          |              3.0            |
*    +------------------------------+-------------------------------+-----------------------------+
*    |          max_answers         |                > 0            |           num_reads         |
*    +------------------------------+-------------------------------+-----------------------------+
*    |           num_reads          |                > 0            |               1             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |        use_random_seed       |              [0 1]            |               0             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |          random_seed         |                >= 0           |          randomly set       |
*    +------------------------------+-------------------------------+-----------------------------+
*/
typedef struct sapi_SwSampleSolverParameters
{
  const int parameter_unique_id;
  sapi_SolverParameterAnswerMode answer_mode;
  double beta;
  int max_answers;
  int num_reads;
  int use_random_seed;
  unsigned int random_seed;
} sapi_SwSampleSolverParameters;

/**
* \brief sapi_SwSampleSolverParameters default value.
*/
DWAVE_SAPI extern const sapi_SwSampleSolverParameters SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;

/**
* \brief sapi software solver (optimize) parameters
*
* parameter_unique_id is the unique id for sapi_SwOptimizeSolverParameters structure.
*                     Trying to modify it will cause undefined behaviour.
* answer_mode indicates whether to return a histogram of answers, sorted in order of energy
*             (SAPI_HISTOGRAM); or to return all answers individually in the order they were
*             read (SAPI_ANSWER_MODE_RAW). (must be SAPI_ANSWER_MODE_HISTOGRAM or SAPI_ANSWER_MODE_RAW,
*             default = SAPI_ANSWER_MODE_HISTOGRAM)
* max_answers maximum number of answers returned from the solver in histogram mode (which
*             sorts the returned states in order of increasing energy); this is the total
*             number of distinct answers. In raw mode, this limits the returned values to
*             the first max_answers of num_reads samples. Thus, in this mode, max_answers
*             should never be more than num_reads. (must be an integer > 0, default = num_reads)
* num_reads a positive integer that indicates the number of states (output solutions) to
*           read from the solver in each programming cycle. (must be an integer > 0, default = 1)
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +------------------------------+-------------------------------+-----------------------------+
*    |            Field             |               Range           |         Default value       |
*    +==============================+===============================+=============================+
*    |         answer_mode          | SAPI_ANSWER_MODE_HISTOGRAM or | SAPI_ANSWER_MODE_HISTOGRAM  |
*    |                              |     SAPI_ANSWER_MODE_RAW      |                             |
*    +------------------------------+-------------------------------+-----------------------------+
*    |          max_answers         |                > 0            |           num_reads         |
*    +------------------------------+-------------------------------+-----------------------------+
*    |           num_reads          |                > 0            |               1             |
*    +------------------------------+-------------------------------+-----------------------------+
*/
typedef struct sapi_SwOptimizeSolverParameters
{
  const int parameter_unique_id;
  sapi_SolverParameterAnswerMode answer_mode;
  int max_answers;
  int num_reads;
} sapi_SwOptimizeSolverParameters;

/**
* \brief sapi_SwOptimizeSolverParameters default value.
*/
DWAVE_SAPI extern const sapi_SwOptimizeSolverParameters SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;


/**
* \brief sapi software solver (heuristic) parameters
*
* parameter_unique_id is the unique id for sapi_SwHeuristicSolverParameters structure.
*                     Trying to modify it will cause undefined behaviour.
* iteration_limit is the maximum number of solver iterations. This does not include the initial
*                 local search. (must be an integer >= 0, default = 10)
* min_bit_flip_prob, max_bit_flip_prob is the bit flip probability range. The probability of
*    flipping each bit is constant for each perturbed solution copy but varies across
*    copies. The probabilities used are linearly interpolated between min_bit_flip_prob
*    and max_bit_flip_prob. Larger values allow more exploration of the solution space
*    and easier escapes from local minima but may also discard nearly-optimal solutions.
*    (must be a number [0.0 1.0] and min_bit_flip_prob <= max_bit_flip_prob, default
*     min_bit_flip_prob = 1.0 / 32.0, default max_bit_flip_prob = 1.0 / 8.0)
* max_local_complexity is the maximum complexity of subgraphs used during local search.
*                      The run time and memory requirements of each step in the local
*                      search are exponential in this parameter. Larger values allow larger
*                      subgraphs (which can improve solution quality) but require much more
*                      time and space.
*                      Subgraph "complexity" here means treewidth + 1.
*                      (must be an integer > 0, default = 9)
* local_stuck_limit is the number of consecutive local search steps that do not improve solution
*                   quality to allow before determining a solution to be a local optimum. Larger
*                   values produce more thorough local searches but increase run time.
*                   (must be an integer > 0, default = 8)
* num_perturbed_copies is the number of perturbed solution copies created at each iteration.
*                      Run time is linear in this value.
*                      (must be an integer > 0, default = 4)
* num_variables is the lower bound on the number of variables. This solver can accept problems
*               of arbitrary structure and the size of the solution returned is determined by
*               the maximum variable index in the problem. The size of the solution can be
*               increased by setting this parameter.
*               (must be an integer >= 0, default = 0)
* use_random_seed indicates whether to use the random_seed field or not.
*                 (must be an integer [0 1], default = 0)
* random_seed (optional) is the random number generator seed. When a value is provided, solving
*                        the same problem with the same parameters will produce the same results
*                        every time. If no value is provided, a time-based seed is selected.
*                        The use of a wall clock-based timeout may in fact cause different results
*                        with the same random_seed value. If the same problem is run under different
*                        CPU load conditions (or on computers with different performance), the amount
*                        of work completed may vary despite the fact that the algorithm is deterministic.
*                        If repeatability of results is important, rely on the iteration_limit parameter
*                        rather than the time_limit_seconds parameter to set the stopping criterion.
*                        (must be an integer >= 0, default is a time-based seed)
* time_limit_seconds is the maximum wall clock time in seconds. Actual run times will
*                    exceed this value slightly.
*                    (must be a number >= 0.0, default = 5.0)
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +------------------------------+-------------------------------+-------------------+
*    |            Field             |               Range           |   Default value   |
*    +==============================+===============================+===================+
*    |        iteration_limit       |               >= 0            |         10        |
*    +------------------------------+-------------------------------+-------------------+
*    |       min_bit_flip_prob      |            [0.0 1.0]          |     1.0 / 32.0    |
*    +------------------------------+-------------------------------+-------------------+
*    |       max_bit_flip_prob      |    [min_bit_flip_prob 1.0]    |     1.0 / 8.0     |
*    +------------------------------+-------------------------------+-------------------+
*    |     max_local_complexity     |               > 0             |         9         |
*    +------------------------------+-------------------------------+-------------------+
*    |       local_stuck_limit      |               > 0             |         8         |
*    +------------------------------+-------------------------------+-------------------+
*    |     num_perturbed_copies     |               > 0             |         4         |
*    +------------------------------+-------------------------------+-------------------+
*    |        num_variables         |               >= 0            |         0         |
*    +------------------------------+-------------------------------+-------------------+
*    |        use_random_seed       |              [0 1]            |         0         |
*    +------------------------------+-------------------------------+-------------------+
*    |          random_seed         |               >= 0            |   randomly set    |
*    +------------------------------+-------------------------------+-------------------+
*    |      time_limit_seconds      |               >= 0.0          |        5.0        |
*    +------------------------------+-------------------------------+-------------------+
*/
typedef struct sapi_SwHeuristicSolverParameters
{
  const int parameter_unique_id;
  int iteration_limit;
  double max_bit_flip_prob;
  int max_local_complexity;
  double min_bit_flip_prob;
  int local_stuck_limit;
  int num_perturbed_copies;
  int num_variables;
  int use_random_seed;
  unsigned int random_seed;
  double time_limit_seconds;
} sapi_SwHeuristicSolverParameters;

/**
* \brief sapi_SwHeuristicSolverParameters default value.
*/
DWAVE_SAPI extern const sapi_SwHeuristicSolverParameters SAPI_SW_HEURISTIC_SOLVER_DEFAULT_PARAMETERS;

/**
* \brief sapi find embedding parameters
*
* fast_embedding tries to get an embedding quickly, without worrying about chain length.
*                (must be an integer [0 1], default = 0)
* max_no_improvement number of rounds of the algorithm to try from the current
*                    solution with no improvement. Each round consists of an attempt to find an
*                    embedding for each variable of S such that it is adjacent to all its
*                    neighbours.
*                    (must be an integer >= 0, default = 10)
* use_random_seed indicates whether to use the random_seed field or not. (must be an integer [0 1], default = 0)
* random_seed seed for random number generator that sapi_findEmbedding uses
*             (must be an integer >= 0, default is a time-based seed)
* timeout the algorithm gives up after timeout seconds.
*         (must be a number >= 0.0, default is approximately 1000.0 seconds)
* tries the algorithm stops after this number of restart attempts
*       (must be an integer >= 0, default = 10)
* verbose control the output information
*         (must be an integer [0 1], default = 0)
*
*      when verbose is 1, the output information will be like:
*
*      component ..., try ...:
*      max overfill = ..., num max overfills = ...
*      Embedding found. Minimizing chains...
*      max chain size = ..., num max chains = ..., qubits used = ...
*
*      detailed explanation of the output information:
*
*           component: process ith (0-based) component, the algorithm tries to embed
*                      larger strongly connected components first, then smaller components
*      try: jth (0-based) try
*      max overfill: largest number of variables represented in a qubit
*      num max overfills: the number of qubits that has max overfill
*      max chain size: largest number of qubits representing a single variable
*      num max chains: the number of variables that has max chain size
*      qubits used: the total number of qubits used to represent variables
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +-----------------------+-----------------+-------------------+
*    |          Field        |      Range      |   Default value   |
*    +=======================+=================+===================+
*    |    fast_embedding     |      [0 1]      |         0         |
*    +-----------------------+-----------------+-------------------+
*    |  max_no_improvement   |       >= 0      |        10         |
*    +-----------------------+-----------------+-------------------+
*    |    use_random_seed    |      [0 1]      |         0         |
*    +-----------------------+-----------------+-------------------+
*    |      random_seed      |      >= 0       |    randomly set   |
*    +-----------------------+-----------------+-------------------+
*    |        timeout        |      >= 0.0     |      1000.0       |
*    +-----------------------+-----------------+-------------------+
*    |         tries         |      >= 0       |        10         |
*    +-----------------------+-----------------+-------------------+
*    |        verbose        |      [0 1]      |         0         |
*    +-----------------------+-----------------+-------------------+
*/
typedef struct sapi_FindEmbeddingParameters
{
  int fast_embedding;
  int max_no_improvement;
  int use_random_seed;
  unsigned int random_seed;
  double timeout;
  int tries;
  int verbose;
} sapi_FindEmbeddingParameters;

/**
* \brief sapi_FindEmbeddingParameters default value.
*/
DWAVE_SAPI extern const sapi_FindEmbeddingParameters SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;

/**
* \brief sapi qsage parameters
*
* draw_sample if 0, sapi_solveQSage will not draw samples, will only do tabu search
*             (must be an integer [0 1], default = 1)
* exit_threshold_value if best value found by sapi_solveQSage <= exit_threshold_value then exit
*                      (can be any number, default = -infinity)
* initial_solution if provided, must only contain -1/1 if ising_qubo parameter
*                  is not set or set as SAPI_PROBLEM_TYPE_ISING; or 0/1 if ising_qubo parameter is
*           set as SAPI_PROBLEM_TYPE_QUBO. Its length must be num_vars. (default is randomly set)
* ising_qubo If set as SAPI_PROBLEM_TYPE_ISING, the return best solution will be -1/1; if set as
*            SAPI_PROBLEM_TYPE_QUBO, the return best solution will be 0/1.
*            (must be SAPI_PROBLEM_TYPE_ISING or SAPI_PROBLEM_TYPE_QUBO, default = SAPI_PROBLEM_TYPE_ISING)
* lp_solver a solver that can solve linear programming problems, refer to the documentation
*            of sapi_QSageLPSolver. (default uses Coin-or Linear Programming solver)
* lp_solver_extra_arg user can pass extra argument for lp_solver. (default = NULL)
* max_num_state_evaluations the maximum number of state evaluations, if the total number of
*                           state evaluations >= max_num_state_evaluations then exit
*                           (must be an integer >= 0, default = 50,000,000)
* use_random_seed indicates whether to use the random_seed field or not.
*                 (must be an integer [0 1], default = 0)
* random_seed seed for random number generator that sapi_solveQSage uses
*             (must be an integer >= 0, default is a time-based seed)
* timeout timeout for sapi_solveQSage (seconds)
*         (must be a number >= 0.0, default is approximately 10.0 seconds)
* verbose control the output information (must be an integer [0 2], default = 0)
*         0: quiet
*         1, 2: different levels of verbosity
*
*         when verbose is 1, the output information will be like:
*
*           [num_state_evaluations = ..., num_obj_func_calls = ...,
*           num_solver_calls = ..., num_lp_solver_calls = ...],
*           best_energy = ..., distance to best_energy = ...
*
*         detailed explanation of the output information:
*
*           num_state_evaluations: the current total number of state evaluations
*           num_obj_func_calls: the current total number of objective function calls
*           num_solver_calls: the current total number of solver (loca/remote) calls
*           num_lp_solver_calls: the current total number of lp solver calls
*           best_energy: the global best energy found so far
*           distance to best_energy: the hamming distance between the global best state
*                                    found so far and the current state found by tabu search
*
*         when verbose is 2, in addition to the output information when verbose is 1,
*         the following output information will also be shown:
*
*           sample_num = ...
*           min_energy = ...
*           move_length = ...
*
*         detailed explanation of the output information:
*
*           sample_num: the number of unique samples returned by sampler
*           min_energy: minimum energy found during the current phase of tabu search
*           move_length: the length of the move (the hamming distance between the current
*                        state and the new state)
*
* The acceptable range and the default value of each field are given in the table below:
*
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |            Field             |              Range             |                Default value                |
*    +==============================+================================+=============================================+
*    |         draw_sample          |              [0 1]             |                      1                      |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |    exit_threshold_value      |            any number          |                  -infinity                  |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |      initial_solution        |               N/A              |                 randomly set                |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |         ising_qubo           |   SAPI_PROBLEM_TYPE_ISING or   |            SAPI_PROBLEM_TYPE_ISING          |
*    |                              |     SAPI_PROBLEM_TYPE_QUBO     |                                             |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |          lp_solver           |               N/A              |   uses Coin-or Linear Programming solver    |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |     lp_solver_extra_arg      |               N/A              |                     NULL                    |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |  max_num_state_evaluations   |               >= 0             |                  50,000,000                 |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |       use_random_seed        |               [0 1]            |                       0                     |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |         random_seed          |               >= 0             |                  randomly set               |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |           timeout            |               >= 0.0           |                      10.0                   |
*    +------------------------------+--------------------------------+---------------------------------------------+
*    |           verbose            |               [0 2]            |                       0                     |
*    +------------------------------+--------------------------------+---------------------------------------------+
*/
typedef struct sapi_QSageParameters
{
  int draw_sample;
  double exit_threshold_value;
  const int* initial_solution;
  sapi_ProblemType ising_qubo;
  sapi_QSageLPSolver lp_solver;
  void* lp_solver_extra_arg;
  long long int max_num_state_evaluations;
  int use_random_seed;
  unsigned int random_seed;
  double timeout;
  int verbose;
} sapi_QSageParameters;

/**
* \brief sapi_QSageParameters default value.
*/
DWAVE_SAPI extern const sapi_QSageParameters SAPI_QSAGE_DEFAULT_PARAMETERS;


/* struct */

/**
* \brief sapi connection struct.
*
* use sapi_freeConnection function to release sapi_Connection pointer.
*/
typedef struct sapi_Connection sapi_Connection;

/**
* \brief sapi solver struct.
*
* use sapi_freeSolver function to release sapi_Solver pointer.
*/
typedef struct sapi_Solver sapi_Solver;

/**
* \brief sapi submitted problem struct.
*
* use sapi_freeSubmittedProblem function to release sapi_SubmittedProblem pointer.
*/
typedef struct sapi_SubmittedProblem sapi_SubmittedProblem;

/**
* \brief sapi quantum solver property's coupler struct.
*
* [q1, q2] represents a coupler (q1 < q2).
*/
typedef struct sapi_Coupler
{
  int q1;
  int q2;
} sapi_Coupler;

/**
* \brief sapi solver supported problem type property struct.
*
* elements is an array of strings representing supported problem types.
* len is the length of the elements array.
*/
typedef struct sapi_SupportedProblemTypeProperty
{
  const char* const* elements;
  size_t len;
} sapi_SupportedProblemTypeProperty;

/**
* \brief sapi quantum solver property struct.
*
* num_qubits is the total number of qubits.
* qubits is an array of active qubits.
* num_active_qubits is the number of active qubits. It is the length of the qubits array.
* couplers is an array of sapi_Coupler.
* num_couplers is the number of couplers. It is the length of the couplers array.
*/
typedef struct sapi_QuantumSolverProperties
{
  int num_qubits;
  const int* qubits;
  size_t qubits_len;
  const sapi_Coupler* couplers;
  size_t couplers_len;
} sapi_QuantumSolverProperties;

/**
* \brief sapi ising range property struct.
*
* h_min is the minimum value h can have.
* h_max is the maximum value h can have.
* j_min is the minimum value j can have.
* j_max is the maximum value j can have.
*/
typedef struct sapi_IsingRangeProperties
{
  double h_min;
  double h_max;
  double j_min;
  double j_max;
} sapi_IsingRangeProperties;

/**
* \brief Anneal offset range.
*
* min: minimum allowed anneal offset value.
* max: maximum allowed anneal offset value.
*/
typedef struct sapi_AnnealOffsetRange
{
  double min;
  double max;
} sapi_AnnealOffsetRange;

/**
* \brief Anneal offset properties.
*
* ranges: array of ranges of valid anneal offsets for each qubit.
* ranges_len: length of ranges. This value should be the total number
*     of qubits: sapi_QuantumSolverProperties.num_qubits.
* step: quantization step size of anneal offset values.
* step_phi0: quantization step size in physical units.
*/
typedef struct sapi_AnnealOffsetProperties
{
  const sapi_AnnealOffsetRange* ranges;
  size_t ranges_len;
  double step;
  double step_phi0;
} sapi_AnnealOffsetProperties;

/**
* \brief Valid parameter names.
*
* elements: array of valid solver parameter names. These will be sorted in ascending order.
* len: length of elements array
*/
typedef struct sapi_ParametersProperty
{
  const char* const* elements;
  size_t len;
} sapi_ParametersProperty;

/**
* \brief Anneal schedule properties
*
* max_points is the maximum number of points allowed in an annealing schedule
* min_annealing_time is the minimum allowed annealing time in microseconds
* min_annealing_time is the maximum allowed annealing time in microseconds
*/
typedef struct sapi_AnnealScheduleProperties
{
  int max_points;
  double min_annealing_time;
  double max_annealing_time;
} sapi_AnnealScheduleProperties;

/**
* \brief Virtual graph properties
*
* extended_j_min and extended_j_max define the extended J range, which is a larger range
*   of allowed J values but subject to the constraints of per_qubit_coupling_{min, max}
* per_qubit_coupling_range_min and per_qubit_coupling_range_max define the allowed
*   coupling on a single qubit, i.e. the sum of all J_ij and J_ji for qubit i
*/
typedef struct sapi_VirtualGraphProperties
{
  double extended_j_min;
  double extended_j_max;
  double per_qubit_coupling_min;
  double per_qubit_coupling_max;
} sapi_VirtualGraphProperties;

/**
* \brief sapi solver property struct.
*
* If any of the property does not exist, it will be a NULL pointer.
*/
typedef struct sapi_SolverProperties
{
  const sapi_SupportedProblemTypeProperty* supported_problem_types;
  const sapi_QuantumSolverProperties* quantum_solver;
  const sapi_IsingRangeProperties* ising_ranges;
  const sapi_AnnealOffsetProperties* anneal_offset;
  const sapi_AnnealScheduleProperties* anneal_schedule;
  const sapi_ParametersProperty* parameters;
  const sapi_VirtualGraphProperties* virtual_graph;
} sapi_SolverProperties;

/**
* \brief sapi problem entry struct.
*
* Note that for when i == j, it represents a linear term.
*/
typedef struct sapi_ProblemEntry
{
  int i;
  int j;
  double value;
} sapi_ProblemEntry;

/**
* \brief sapi problem struct.
*
* elements is an array of sapi_ProblemEntry struct.
* len is the length of the elements array.
*/
typedef struct sapi_Problem
{
  sapi_ProblemEntry* elements;
  size_t len;
} sapi_Problem;

/**
* \brief status of an asynchronous submitted problem
*
* Use sapi_asyncStatus to fill in this structure.
*
* problem_id: remote problem ID (null terminated). Will be empty until
*     problem is submitted or if using a local solver.
* time_received: time at which the server received the problem (ISO-8601 format).  Will be
*     empty until problem is submitted.
* time_solved: time at which the problem was completed (ISO-8601 format).  Will be empty until
*     problem is completed.
* state: describes the state of the problem, as seen by the client library. One of
*     - SAPI_STATE_SUBMITTING: problem is still being submitted
*     - SAPI_STATE_SUBMITTED: problem has been submitted but isn't done yet
*     - SAPI_STATE_DONE: problem is done (completed, failed, or cancelled)
*     - SAPI_STATE_FAILED: a network communication error occurred while submitting the problem
*           or checking its status and polling the server has stopped.  This does not indicate
*           that solving the problem has failed!
*     - SAPI_STATE_RETRYING: a network communication error occurred but submission/polling is
*           being retried (either automatically or by a call to sapi_asyncRetry)
* last_good_state: the last "good" value of state, that is, the last value of
*     state other than SAPI_STATE_FAILED or SAPI_STATE_RETRYING.
* remote_status: the status of the problem as reported by the server. One of
*     - SAPI_STATUS_UNKNOWN: no server response yet (still submitting)
*     - SAPI_STATUS_PENDING: problem is waiting in a queue
*     - SAPI_STATUS_IN_PROGRESS: problem is being solved (or will be solved shortly)
*     - SAPI_STATUS_COMPLETED: solving succeeded
*     - SAPI_STATUS_FAILED: solving failed
*     - SAPI_STATUS_CANCELED: problem cancelled by user
* error_code: error type when in any kind of failed state:
*     - state is either SAPI_STATE_RETRYING or SAPI_STATE_FAILED
*     - remote_status is either SAPI_STATUS_FAILED or SAPI_STATUS_CANCELED
*     Otherwise this value will be SAPI_OK.
* error_message: error message when error_code is not SAPI_OK (blank otherwise).
*
* This structure isn't meaningful for problems running locally. Field values will be:
*     - problem_id: blank
*     - state: SAPI_STATE_DONE
*     - last_good_state: SAPI_STATE_DONE
*     - remote_status: SAPI_STATUS_COMPLETED
*     - error_code: SAPI_OK
*     - error_message: blank
*/
typedef struct sapi_ProblemStatus
{
  char problem_id[64];
  char time_received[64];
  char time_solved[64];
  sapi_SubmittedState state;
  sapi_SubmittedState last_good_state;
  sapi_RemoteStatus remote_status;
  sapi_Code error_code;
  char error_message[SAPI_ERROR_MESSAGE_MAX_SIZE];
} sapi_ProblemStatus;

typedef struct sapi_FixedVariable {
  int var;
  int value;
} sapi_FixedVariable;

typedef struct sapi_FixVariablesResult
{
  sapi_FixedVariable *fixed_variables;
  size_t fixed_variables_len;
  double offset;
  sapi_Problem new_problem;
} sapi_FixVariablesResult;


/* Problem embedded by sapi_embedProblem
 *
 * problem: embedded original problem
 * jc: chain edges, i.e. J values coupling vertices representing the same.
 *   logical variable.
 * embeddings: original embeddings, possibly modified by cleaning or smearing.
 */
typedef struct sapi_EmbedProblemResult
{
  sapi_Problem problem;
  sapi_Problem jc;
  sapi_Embeddings embeddings;
} sapi_EmbedProblemResult;

/* Quantum processor timing information.
 *
 * All times are in microseconds.  A value of -1 indicates a value not provided.
 *
 * qpu_access_time: total exclusive chip access time.  Made up of programming
 *     time and sampling time.
 * qpu_programming_time: time to program the problem on the chip.
 * qpu_sampling_time: time to anneal and read out all samples.
 * qpu_anneal_time_per_sample: time to anneal each sample.
 * qpu_readout_time_per_sample: time to read out each sample.
 * qpu_delay_time_per_sample: delay time between samples.
 * total_post_processing_time: total time spent in post-processing (including
 *     energy calculations and histogramming)
 * post_processing_overhead_time: part of total_post_processing_time that is
 *     not concurrent with quantum processor
 *
 * total_real_time: deprecated.  Use qpu_access_time.
 * run_time_chip: deprecated.  Use qpu_sampling_time.
 * anneal_time_per_run: deprecated.  Use qpu_anneal_time_per_sample.
 * readout_time_per_run: deprecated.  Use qpu_readout_time_per_sample.
 */
typedef struct sapi_Timing
{
  long long qpu_access_time;
  long long qpu_programming_time;
  long long qpu_sampling_time;
  long long qpu_anneal_time_per_sample;
  long long qpu_readout_time_per_sample;
  long long qpu_delay_time_per_sample;
  long long total_post_processing_time;
  long long post_processing_overhead_time;

  long long run_time_chip;
  long long anneal_time_per_run;
  long long readout_time_per_run;
  long long total_real_time;
} sapi_Timing;

/**
* \brief sapi ising/qubo result struct.
*
* free_function
* type
* solutions is an array of integers. The solutions contain either -1/1 for ising problem
*           or 0/1 for qubo problem. Note that its length is: solution_len * num_solutions.
*           If answer_mode is SAPI_ANSWER_MODE_HISTOGRAM: the states (entries) are unique and sorted in
*           increasing-energy order;
*           If answer_mode is SAPI_ANSWER_MODE_RAW: all the output states are in the order that they
*           were generated.
* solution_len is the length of each solution.
* num_solutions is the number of solutions.
* energies is an array of doubles representing the corresponding energy for each solution.
*          Note that energies' length is: num_solutions.
* num_occurrences is an array of integers representing the number of occurrences for each
*                 solution. This field will be NULL if answer_mode is *SAPI_ANSWER_MODE_RAW*. Note that
*                 if num_occurrences pointer is not NULL, its length is: num_solutions.
* timing is a sapi_Timing struct. Fields are solver-dependent, containing the time taken
*        (in microseconds) at each step of the routine such as "anneal_time_per_run",
*        "preprocessing_time", etc. Only hardware solvers return the timing parameter.
*/
typedef struct sapi_IsingResult
{
  int* solutions;
  size_t solution_len;
  size_t num_solutions;
  double* energies;
  int* num_occurrences;
  sapi_Timing timing;
} sapi_IsingResult;

/**
* \brief qsage objective function related parameter struct
*
* objective_function function pointer of sapi_QSageObjectiveFunction
* objective_function_extra_arg objective function's extra argument
* num_vars number of variables
*/
typedef struct sapi_QSageObjFunc
{
  sapi_QSageObjectiveFunction objective_function;
  void* objective_function_extra_arg;
  int num_vars;
} sapi_QSageObjFunc;

/**
* \brief sapi qsage stat struct
*
* num_state_evaluations is the number of state evaluations.
* num_obj_func_calls is the number of user-provided objective function calls.
* num_solver_calls is the number of solver (local/remote) calls.
* num_lp_solver_calls is the number of lp solver calls.
*/
typedef struct sapi_QSageStat
{
  long long int num_state_evaluations;
  long long int num_obj_func_calls;
  long long int num_solver_calls;
  long long int num_lp_solver_calls;
} sapi_QSageStat;

/**
* \brief sapi qsage progress struct. This struct stores qsage progress history.
*
* stat is a sapi_QSageStat struct representing the qsage stat when energy was found.
* time is the time (seconds) when energy was found.
* energy is the objective value of a certain state of the objective function.
*/
typedef struct sapi_QSageProgressEntry
{
  sapi_QSageStat stat;
  double time;
  double energy;
} sapi_QSageProgressEntry;

/**
* \brief sapi qsage progress struct.
*
* elements is an array of sapi_QSageProgressEntry struct.
* len is the length of the elements array.
*/
typedef struct sapi_QSageProgress
{
  sapi_QSageProgressEntry* elements;
  size_t len;
} sapi_QSageProgress;

/**
* \brief sapi qsage info struct.
*
* stat is a sapi_QSageStat struct which stores the total stat when sapi_solveQSage completes
*      computation.
* state_evaluations_time is the state evaluations time (seconds).
* solver_calls_time is the solver (local/remote) calls time (seconds).
* lp_solver_calls_time is the lp solver calls time (seconds).
* total_time is the total running time of sapi_solveQSage (seconds).
* progress is a sapi_QSageProgress struct which stores the sapi_solveQSage progress history
*          during the computation.
*/
typedef struct sapi_QSageInfo
{
  sapi_QSageStat stat;
  double state_evaluations_time;
  double solver_calls_time;
  double lp_solver_calls_time;
  double total_time;
  sapi_QSageProgress progress;
} sapi_QSageInfo;

/**
* \brief qsage result struct.
*
* best_solution is the best state founnd.
* len is the length of the best_solution array.
* best_energy is the best energy found.
* info is the sapi_QSageInfo struct which stores the information during the
*      sapi_solveQSage computation.
*/
typedef struct sapi_QSageResult
{
  int* best_solution;
  size_t len;
  double best_energy;
  sapi_QSageInfo info;
} sapi_QSageResult;


/**
* \brief sapi term entry struct.
*
* terms is an array of term. Must only contain non negative integers.
* len is the length of the terms array.
*/
typedef struct sapi_TermsEntry
{
  int* terms;
  size_t len;
} sapi_TermsEntry;

/**
* \brief sapi terms struct.
*
* elements is an array of sapi_TermEntry struct.
* len is the length of the elements array.
*/
typedef struct sapi_Terms
{
  sapi_TermsEntry* elements;
  size_t len;
} sapi_Terms;

/**
* \brief sapi variables rep entry struct.
*
* variable is the newly introduced variable v.
* rep is an array which contains two variables x, y so that v = x * y.
*/
typedef struct sapi_VariablesRepEntry
{
  int variable;
  int rep[2];
} sapi_VariablesRepEntry;

/**
* \brief sapi variables rep struct.
*
* elements is an array of sapi_VariablesRepEntry struct.
* len is the length of the elements array.
*/
typedef struct sapi_VariablesRep
{
  sapi_VariablesRepEntry* elements;
  size_t len;
} sapi_VariablesRep;


/* function */

/**
* \brief sapi global initialization function.
*
* Call this function to initialize before using any function in this dwave_sapi library.
*/
DWAVE_SAPI sapi_Code sapi_globalInit();

/**
* \brief sapi global cleanup function.
*
* After finish using functions in this dwave_sapi library, call this function to clean up.
*/
DWAVE_SAPI void sapi_globalCleanup();

/**
* \brief sapi local connection.
*
* \return a sapi_Connection pointer which can be used in functions:
*         sapi_listSolvers, sapi_getSolver.
*
* the sapi_Connection pointer that this function returns does not need to be freed by
* sapi_freeConnection function. It will be automatically freed when calling
* sapi_globalCleanup function.
*/
DWAVE_SAPI sapi_Connection* sapi_localConnection();

/**
* \brief sapi remote connection.
*
* \param url a string for url.
* \param token a string for token.
* \param proxy_url a string for proxy url. (If do not want to use proxy,
*        set proxy_url as NULL.)
*        proxy_url causes requests to go through a proxy. If proxy is given, it must be
*        a string url of proxy. The default is to read the list of proxies
*        from the environment variables <protocol>_proxy. If no proxy environment
*        variables are set, in a Windows environment, proxy settings are obtained
*        from the registry's Internet Settings section and in a Mac OS X environment,
*        proxy information is retrieved from the OS X System Configuration Framework.
*     (To disable autodetected proxy pass an empty string.)
* \param remote_connection a sapi_Connection pointer which can be used in functions:
*        sapi_listSolvers, sapi_getSolver.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeConnection function to release the sapi_Connection pointer that
* this function returns.
*/
DWAVE_SAPI sapi_Code sapi_remoteConnection(const char* url, const char* token, const char* proxy_url, sapi_Connection** remote_connection, char* err_msg);

/**
* \brief list solvers available from a connection.
*
* \param connection returned by either sapi_localConnection or sapi_remoteConnection.
* \return a string array which contains all available solvers' names, the last element
*         will be NULL.
*/
DWAVE_SAPI const char** sapi_listSolvers(const sapi_Connection* connection);

/**
* \brief get a solver.
*
* \param connection returned by either sapi_localConnection or sapi_remoteConnection.
* \param solver_name the name of the requested solver. Must be listed in sapi_listSolvers.
* \return solver the request solver pointer. The solver pointer can be used by
*         sapi_getSolverProperties to get the property of the solver. It can also be used by
*         sapi_solve to solve ising/qubo problems synchronously and sapi_asyncSolve to
*         solve ising/qubo problems asynchronously. If no solver's name is solver_name,
*         return NULL.
*
* use sapi_freeSolver function to release the sapi_Solver pointer that this function returns.
*/
DWAVE_SAPI sapi_Solver* sapi_getSolver(const sapi_Connection* connection, const char* solver_name);

/**
* \brief fix variables.
*
* \param problem must be a QUBO.
* \param fix_variables_method the method for fix variables algorithms, refer to the sapi_FixVariablesMethod
*        comments
* \param fix_variables_result fix variables results
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeFixVariablesResult function to release the sapi_FixVariablesResult pointer that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_fixVariables(const sapi_Problem* problem, sapi_FixVariablesMethod method, sapi_FixVariablesResult** result, char* err_msg);

/* Embed an Ising problem into a given graph.
 *
 * problem: Ising problem to embed.
 * embeddings: mapping of problem variables to graph vertices.
 * adj: adjacency matrix of target graph.  Uses sapi_Problem data structure,
 *   but only the i and j fields of each sapi_ProblemEntry are used (value is
 *   ignored).
 * clean: boolean value.  "Cleaning" an embedding means iteratively removing
 *   vertices that are:
 *     * adjacent to a single vertex representing the same variable
 *     * not adjacent to any other embedded variables
 * smear: boolean value.  "Smearing" an embedding means attempts to lower the
 *   scale of h values compared to those of J values (relative to their
 *   respective ranges) by adding more vertices to variables with large h
 *   values.  Smearing is done after cleaning, so it is potentially useful to
 *   do both.
 * ranges: h and J ranges.  Only used when smearing is enabled.  May be NULL,
 *   in which case both ranges are assumed to be [-1, 1]
 * result: embedded problem and possibly modified embedding.  Returned
 *   embedding will only differ from passed embedding if cleaning or smearing
 *   is enabled.
 * err_msg: a buffer of size at least SAPI_ERROR_MESSAGE_MAX_SIZE.  Error
 *   message will be copied here if the function fails.  May be NULL.
 *
 * Use the sapi_freeEmbedProblemResult function to release result memory.
 */
DWAVE_SAPI sapi_Code sapi_embedProblem(
    const sapi_Problem* problem,
    const sapi_Embeddings* embeddings,
    const sapi_Problem* adj,
    int clean,
    int smear,
    const sapi_IsingRangeProperties* ranges,
    sapi_EmbedProblemResult** result,
    char* err_msg);

/*
 * "Unembed" solutions from an embedded problem back to solutions for
 * the original problem.
 *
 * solutions, solution_len, num_solutions: same as corresponding fields in
 *   sapi_IsingResult structure.
 * embeddings: same embeddings as used to embed the original problem.
 *   Note: if you used sapi_embedProblem with cleaning or smearing enabled,
 *   be sure to use the returned embeddings.
 * broken_chains: strategy for repairing broken chains.  The solution bits
 *   representing a single variable (a "chain") may not agree.  This
 *   parameter controls how the value is chosen.
 * problem: original problem before embedding.  Required when broken_chains
 *   is SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, ignored otherwise.
 * new_solutions: array of size num_solutions * (# of original variables)
 *   that will be filled with unembedded solutions.
 * num_new_solutions: output value that will be set to the number of new
 *   solutions.  This will equal num_solutions unless broken_chains is
 *   SAPI_BROKEN_CHAINS_DISCARD, in which case it could be less.
 * err_msg: a buffer of size at least SAPI_ERROR_MESSAGE_MAX_SIZE.  Error
 *   message will be copied here if the function fails.  May be NULL.
 */
DWAVE_SAPI sapi_Code sapi_unembedAnswer(
    const int* solutions,
    size_t solution_len,
    size_t num_solutions,
    const sapi_Embeddings* embeddings,
    sapi_BrokenChains broken_chains,
    const sapi_Problem* problem,
    int* new_solutions,
    size_t* num_new_solutions,
    char* err_msg);

/**
* \brief get solver property
*
* \param solver a solver pointer. Returned by the sapi_getSolver function.
* \return a pointer of sapi_SolverProperties struct, contents are solver-specific,
*         see documentation.
* \All solvers will have a "supported_problem_types" parameter whose value is an array of
* \problem type strings.
*
* Note that this sapi_SolverProperties struct pointer will be freed automatically after the
* solver is freed by calling sapi_freeSolver.
*/
DWAVE_SAPI const sapi_SolverProperties* sapi_getSolverProperties(const sapi_Solver* solver);

/**
* \brief solve an ising/qubo problem synchronously with any type of ising/qubo solver
*
* \param solver a sapi_Solver pointer to use to solve the problem.
* \param problem a pointer to sapi_SparseMatrix struct. The type entry in sapi_SparseMatrix
*        struct must be set as 0 if the problem is an ising problem or 1 if the problem is
*        a qubo problem.
* \param solver_params parameters for solver. If the solver is a quantum solver, the
*        solver_params must be a pointer to type sapi_QuantumSolverParameters; if the solver is
*        a software sample solver, the solver_params must be a pointer to type
*        sapi_SwSampleSolverParameters; if the solver is a software optimize solver, the
*        solver_params must be a pointer to type sapi_SwOptimizeSolverParameters, etc.
* \param result pointer of pointer to sapi_IsingResult struct.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeIsingResult function to release the sapi_IsingResult pointer that
* this function returns.
*/
DWAVE_SAPI sapi_Code sapi_solveIsing(const sapi_Solver* solver, const sapi_Problem* problem, const sapi_SolverParameters* solver_params, sapi_IsingResult** result, char* err_msg);

/**
* \brief solve an ising/qubo problem synchronously with any type of ising/qubo solver
*
* \param solver a sapi_Solver pointer to use to solve the problem.
* \param problem a pointer to sapi_SparseMatrix struct. The type entry in sapi_SparseMatrix
*        struct must be set as 0 if the problem is an ising problem or 1 if the problem is
*        a qubo problem.
* \param solver_params parameters for solver. If the solver is a quantum solver, the
*        solver_params must be a pointer to type sapi_QuantumSolverParameters; if the solver is
*        a software sample solver, the solver_params must be a pointer to type
*        sapi_SwSampleSolverParameters; if the solver is a software optimize solver, the
*        solver_params must be a pointer to type sapi_SwOptimizeSolverParameters, etc.
* \param result pointer of pointer to sapi_IsingResult struct.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeIsingResult function to release the sapi_IsingResult pointer that
* this function returns.
*/
DWAVE_SAPI sapi_Code sapi_solveQubo(const sapi_Solver* solver, const sapi_Problem* problem, const sapi_SolverParameters* solver_params, sapi_IsingResult** result, char* err_msg);

/**
* \brief solve an ising/qubo problem asynchronously with any type of ising/qubo solver.
*
* \param solver a sapi_Solver pointer to use to solve the problem.
* \param problem a pointer to sapi_SparseMatrix struct. The type entry in sapi_SparseMatrix
*        struct must be set as 0 if the problem is an ising problem or 1 if the problem is
*        a qubo problem.
* \param solver_params parameters for solver. If the solver is a quantum solver, the
*        solver_params must be a pointer to type sapi_QuantumSolverParameters; if the solver is
*        a software sample solver, the solver_params must be a pointer to type
*        sapi_SwSampleSolverParameters; if the solver is a software optimize solver, the
*        solver_params must be a pointer to type sapi_SwOptimizeSolverParameters, etc.
* \param submitted_problem pointer of pointer to sapi_SubmittedProblem struct.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeSubmittedProblem function to release the sapi_SubmittedProblem pointer
* that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_asyncSolveIsing(const sapi_Solver* solver, const sapi_Problem* problem, const sapi_SolverParameters* solver_params, sapi_SubmittedProblem** submitted_problem, char* err_msg);

/**
* \brief solve an ising/qubo problem asynchronously with any type of ising/qubo solver.
*
* \param solver a sapi_Solver pointer to use to solve the problem.
* \param problem a pointer to sapi_SparseMatrix struct. The type entry in sapi_SparseMatrix
*        struct must be set as 0 if the problem is an ising problem or 1 if the problem is
*        a qubo problem.
* \param solver_params parameters for solver. If the solver is a quantum solver, the
*        solver_params must be a pointer to type sapi_QuantumSolverParameters; if the solver is
*        a software sample solver, the solver_params must be a pointer to type
*        sapi_SwSampleSolverParameters; if the solver is a software optimize solver, the
*        solver_params must be a pointer to type sapi_SwOptimizeSolverParameters, etc.
* \param submitted_problem pointer of pointer to sapi_SubmittedProblem struct.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeSubmittedProblem function to release the sapi_SubmittedProblem pointer
* that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_asyncSolveQubo(const sapi_Solver* solver, const sapi_Problem* problem, const sapi_SolverParameters* solver_params, sapi_SubmittedProblem** submitted_problem, char* err_msg);

/**
* \brief waits for problems to complete
* \param submitted_problems an array of submitted problems, each of the submitted problems
*        returned by sapi_asyncSolve function
* \param num_submitted_problems the length of the submitted_problems array
* \param min_done minimum number of problems that must be completed before returning (without timeout)
* \param timeout maximum time to wait (in seconds)
* \return 1 if returning because enough problems completed, 0 if returning because of timeout
*/
DWAVE_SAPI int sapi_awaitCompletion(const sapi_SubmittedProblem** submitted_problems, size_t num_submitted_problems, size_t min_done, double timeout);

/**
* \brief cancel a submitted problem
* \param submitted_problem a sapi_SubmittedProblem pointer. Returned by the
*        sapi_asyncSolve function.
*/
DWAVE_SAPI void sapi_cancelSubmittedProblem(sapi_SubmittedProblem* submitted_problem);

/**
* \brief check if an asynchronous submitted problem is done.
*
* \param submitted_problem asynchronous submitted problem returned by sapi_asyncSolve.
* \return 0/1 indicating the problem hasn't/has been solved.
*
* Once the problem is done, you can retrieve the answer with the sapi_asyncResult function.
*/
DWAVE_SAPI int sapi_asyncDone(const sapi_SubmittedProblem* submitted_problem);

/**
* \brief retrieve the answer from an ansynchronous submitted problem.
*
* \param submitted_problem sapi_SubmittedProblem pointer returned by sapi_asyncSolve function.
* \param result the answer to the problem. The format will be identical to answers returned by
*        the synchronous solving functions sapi_solve.
*        Attempting to retrieve the answer to a problem that has been cancelled will trigger
*        a SAPI_ERR_PROBLEM_CANCELLED error.
*        Attempting to retrieve the answer to a problem that is not done will trigger a
*        SAPI_ERR_ASYNC_NOT_DONE error.
*        Use sapi_asyncDone function to check whether or not the problem is done.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeIsingResult function to release the result pointer that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_asyncResult(const sapi_SubmittedProblem* submitted_problem, sapi_IsingResult** result, char* err_msg);

/**
* \brief retry a submitted problem that has encountered a network, communication, or authentication error
*
* This function has no effect on problems that:
*   - are still in progress
*   - have been cancelled
*   - failed while solving (e.g. due to an invalid parameter)
* Its purpose is to recover from intermittent network failures (SAPI_ERR_NETWORK but occasionally
* SAPI_ERR_COMMUNICATION) without resubmitting a problem that may have completed successfully.
* It can also recover from authentication errors caused by disabling a token (of course, the token
* must be re-enabled first).
*
* \params submitted_problem the problem to retry
*/
DWAVE_SAPI void sapi_asyncRetry(const sapi_SubmittedProblem* submitted_problem);

/**
* \brief get submitted problem status
*
* See sapi_ProblemStatus definition for explanation.
*
* \param submitted_problem the submitted problem to examine
* \param status pointer to a sapi_ProblemStatus structure that will be filled in
*
* The only non-SAPI_OK return value is SAPI_ERR_OUT_OF_MEMORY, in which case status in unchanged.
*/
DWAVE_SAPI sapi_Code sapi_asyncStatus(const sapi_SubmittedProblem* submitted_problem, sapi_ProblemStatus *status);

/**
* \brief attempts to find an embedding of a qubo/ising problem in a graph. This function
*        is entirely heuristic: failure to return an embedding does not prove that no
%        embedding exists.
*
* \param S edge structures of a problem, can be qubo/ising. The embedder only cares about
*        the edge structure
*        (i.e. which variables have a nontrivial interactions), not the coefficient values.
* \param A an adjacency matrix of the graph.
* \param find_embedding_params parameters for find embedding algorithm.
* \param embeddings arrays of qubits representing logical variables. If the algorithm fails,
*        embeddings' len is 0.
* \param err_msg error message.
* \return sapi embedding error code.
*
* use sapi_freeEmbeddings function to release the embeddings pointer that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_findEmbedding(const sapi_Problem* S, const sapi_Problem* A, const sapi_FindEmbeddingParameters* find_embedding_params, sapi_Embeddings** embeddings, char* err_msg);

/**
* \brief solve a qsage problem.
* (for num_vars <= 10, the sapi_solveQSage will do a brute-force search)
*
* \param obj_func a pointer to sapi_QSageObjFunc.
* \param solver a sapi_Solver pointer.
* \param solver_params parameters for solver.
* \param qsage_params parameters for qsage algorithm.
* \param qsage_result a pointer to a pointer to sapi_QSageResult.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeQSageResult function to release the qsage_result pointer that this function returns.
*/
DWAVE_SAPI sapi_Code sapi_solveQSage(const sapi_QSageObjFunc* obj_func, const sapi_Solver* solver, const sapi_SolverParameters* solver_params, const sapi_QSageParameters* qsage_params, sapi_QSageResult** qsage_result, char* err_msg);

/**
* \brief build the adjacency matrix for the Chimera architecture. The
*        architecture is an M-by-N lattice of elements where each element is a
*        K_{L,L} bipartite graph.
*
* \param M, N, L Chimera dimensions.
* \param A adjacency structure.
* \return sapi error code.
*
* use sapi_freeSparseMatrix function to release the sapi_SparseMatrix pointer that
* this function returns.
*/
DWAVE_SAPI sapi_Code sapi_getChimeraAdjacency(int M, int N, int L, sapi_Problem** A);

/**
* \brief build the adjacency matrix for the solver.
* \param solver a solver pointer returned by the sapi_getSolver function.
* \param A adjacency structure.
* \return sapi error code.
*
* use sapi_freeSparseMatrix function to release the sapi_SparseMatrix pointer that
* this function returns.
*/
DWAVE_SAPI sapi_Code sapi_getHardwareAdjacency(const sapi_Solver* solver, sapi_Problem** A);

/**
* \brief reduce the degree of a set of objectives specified by terms to have maximum two
         degrees via the introduction of ancillary variables.
*
* \param terms each term's variables in the expression, the index in terms must be a
*        non-negative integer.
* \param new_terms terms after using ancillary variables.
* \param variables_rep ancillary variables.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeTerms function to release the sapi_Terms pointer that this function returns.
* use sapi_freeVariablesRep function to release the sapi_VariablesRep pointer that this
* function returns.
*/
DWAVE_SAPI sapi_Code sapi_reduceDegree(const sapi_Terms* terms, sapi_Terms** new_terms, sapi_VariablesRep** variables_rep, char* err_msg);

/**
* \brief given a function f defined over binary variables represented as a
         vector stored in decimal order, e.g. f[000], f[001], f[010], f[011],
         f[100], f[101], f[110], f[111] represent it as a quadratic objective.
*
* \param f a function defined over binary variables represented as an
         array stored in decimal order.
* \param f_len  f's length, must be power of 2.
* \param penalty_weight if this parameter is supplied it sets the strength of the
         penalty used to define the product constaints on the new ancillary
         variables. Set it as NULL if want to use the default value, the default
     value is usually sufficiently large, but may be larger than necessary.
* \param new_terms the terms in the qubo arising from quadraticization of the
         interactions present in f.
* \param variables_rep the definition of the new ancillary variables.
* \param Q quadratic coefficients.
* \param err_msg error message.
* \return sapi error code.
*
* use sapi_freeTerms function to release the sapi_Terms pointer that this function returns.
* use sapi_freeVariablesRep function to release the sapi_VariablesRep pointer that this
* function returns.
* use sapi_freeSparseMatrix function to release the sapi_SparseMatrix pointer that this
* function returns.
*/
DWAVE_SAPI sapi_Code sapi_makeQuadratic(const double* f, size_t f_len, const double* penalty_weight, sapi_Terms** new_terms, sapi_VariablesRep** variables_rep, sapi_Problem** Q, char* err_msg);

/**
* \brief free sapi_Connection pointer.
*
* \param connection returned by sapi_remoteConnection.
*
* Note that the sapi_Connection pointer that is returned by sapi_localConnection does not
* need to be freed by using this function. It will be automatically freed when calling
* sapi_globalCleanup function.
*/
DWAVE_SAPI void sapi_freeConnection(sapi_Connection* connection);

/**
* \brief free sapi_Solver pointer.
*
* \param solver a sapi_Solver pointer.
*/
DWAVE_SAPI void sapi_freeSolver(sapi_Solver* solver);

/**
* \brief free sapi_SubmittedProblem pointer.
*
* \param submitted_problem returned by sapi_asyncSolve.
*/
DWAVE_SAPI void sapi_freeSubmittedProblem(sapi_SubmittedProblem* submitted_problem);

/**
* \brief free sapi_SparseMatrix pointer.
*
* \param sparse_matrix returned by sapi_getChimeraAdjacency, sapi_getHardwareAdjacency
*                      or sapi_makeQuadratic.
*/
DWAVE_SAPI void sapi_freeProblem(sapi_Problem* problem);

/**
* \brief free sapi_IsingResult pointer.
*
* \param result returned by sapi_solve or sapi_asyncResult.
*/
DWAVE_SAPI void sapi_freeIsingResult(sapi_IsingResult* result);

DWAVE_SAPI void sapi_freeEmbedProblemResult(sapi_EmbedProblemResult* embed_problem_result);

/**
* \brief free sapi_Embeddings pointer.
*
* \param embeddings returned by sapi_findEmbedding.
*/
DWAVE_SAPI void sapi_freeEmbeddings(sapi_Embeddings* embeddings);

/**
* \brief free sapi_FixVariablesResult pointer.
*
* \param fix_variables_result returned by sapi_fixVariables.
*/
DWAVE_SAPI void sapi_freeFixVariablesResult(sapi_FixVariablesResult* fix_variables_result);

/**
* \brief free sapi_QSageResult pointer.
*
* \param qsage_result a sapi_QSageResult pointer.
*/
DWAVE_SAPI void sapi_freeQSageResult(sapi_QSageResult* qsage_result);

/**
* \brief free sapi_Terms pointer.
*
* \param terms returned by sapi_reduceDegree or sapi_makeQuadratic.
*/
DWAVE_SAPI void sapi_freeTerms(sapi_Terms* terms);

/**
* \brief free sapi_VariablesRep pointer.
*
* \param variables_rep returned by sapi_reduceDegree or sapi_makeQuadratic.
*/
DWAVE_SAPI void sapi_freeVariablesRep(sapi_VariablesRep* variables_rep);

/**
* \brief returns the sapi version string.
*/
DWAVE_SAPI const char* sapi_version();

#ifdef __cplusplus
}
#endif

#endif /* DWAVE_SAPI_H_INCLUDED */
