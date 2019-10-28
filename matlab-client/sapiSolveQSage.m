% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [bestSolution bestEnergy info] = sapiSolveQSage(objectiveFunction, numVars, solver, solverParams, qsageParams)
%% SAPISOLVEQSAGE Solve a QSage problem.
%
%  [bestSolution bestEnergy info] = sapiSolveQSage(objectiveFunction, numVars, solverParams, qsageParams)
%  (can be interrupted by Ctrl-C, it will return the best solution found so far.)
%  (for numVars <= 10, sapiSolveQSage will do a brute-force search)
%
%  Input parameters
%    objectiveFunction: the implementation of the objective function f.
%      sapiSolveQSage assumes the following signature:
%      y = f(states) where states is a two-dimensional matrix. Each column
%      in states is a state. y is the returned objective value for each state.
%
%    numVars: number of variables.
%
%    solver: a solver handle.
%
%    solverParams: structure of solver parameters.
%
%    qsageParams: structure of QSage parameters.
%
%   parameters for qsageParams:
%
%   draw_sample: if false, sapiSolveQSage will not draw samples, will only do tabu search
%                (must be a boolean, default = true)
%
%   exit_threshold_value : if best value found by sapiSolveQSage <= exit_threshold_value then exit
%                          (can be any number, default = -Inf)
%
%   init_solution: the initial solution for the problem
%                  (must be a matrix containing only -1/1 if "ising_qubo" parameter is not set or set as 'ising';
%                  or 0/1 if "ising_qubo" parameter is set as 'qubo'. The number of elements of init_solution
%                  must be equal to numVars, default is randomly set)
%
%   ising_qubo: if set as 'ising', the return best solution will be -1/1;
%               if set as 'qubo', the return best solution will be 0/1
%               (must be a string 'ising' or 'qubo', default = 'ising')
%
%   lp_solver: a solver that can solve linear programming problems.
%              Finds the minimum of a problem specified by
%                  min      f * x
%                  st.      Aineq * x <= bineq
%                             Aeq * x = beq
%                             lb <= x <= ub
%                  f, bineq, beq, lb, and ub are column vectors.
%                  Aineq and Aeq are matrices.
%
%              sapiSolveQSage assumes the following signature for lp_solver:
%                 x = solve(f, Aineq, bineq, Aeq, beq, lb, ub)
%                 input arguments:
%                    f: linear objective function, double column vector
%			         Aineq: linear inequality constraints, double matrix
%			         bineq: righthand side for linear inequality constraints, double column vector
%			         Aeq: linear equality constraints, double matrix
%			         beq: righthand side for linear equality constraints, double column vector
%			         lb: lower bounds, double column vector
%			         ub: upper bounds, double column vector
%			      output arguments:
%			         x: solution found by the optimization function
%
%              (default uses Coin-or Linear Programming solver)
%
%   max_num_state_evaluations: the maximum number of state evaluations, if the
%                              total number of state evaluations >= max_num_state_evaluations
%                              then exit
%                              (must be an integer >= 0, default = 50,000,000)
%
%   random_seed: seed for random number generator that sapiSolveQSage uses
%                (must be an integer >= 0, default is randomly set)
%
%   timeout: timeout for sapiSolveQSage (seconds, must be a number >= 0, default is approximately 10.0 seconds)
%
%   verbose: control the output information (must be an integer [0 2], default = 0)
%            0: quiet
%            1, 2: different levels of verbosity
%
%            when verbose is 1, the output information will be like:
%
%              [num_state_evaluations = ..., num_obj_func_calls = ..., num_solver_calls = ..., num_lp_solver_calls = ...],
%              best_energy = ..., distance to best_energy = ...
%
%            detailed explanation of the output information:
%
%              num_state_evaluations: the current total number of state evaluations
%              num_obj_func_calls: the current total number of objective function calls
%              num_solver_calls: the current total number of solver calls
%              num_lp_solver_calls: the current total number of lp solver calls
%              best_energy: the global best energy found so far
%              distance to best_energy: the hamming distance between the global best state found so far
%                                       and the current state found by tabu search
%
%            when verbose is 2, in addition to the output information when verbose is 1, the following
%            output information will also be shown:
%
%              sample_num = ...
%              min_energy = ...
%              move_length = ...
%
%            detailed explanation of the output information:
%
%              sample_num: the number of unique samples returned by sampler
%              min_energy: minimum energy found during the current phase of tabu search
%              move_length: the length of the move (the hamming distance between the current state and
%                           the new state)
%
%    The acceptable range and the default value of each field are given in the table below:
%
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |            Field             |              Range             |                Default value                |
%    +==============================+================================+=============================================+
%    |         draw_sample          |          true or false         |                    true                     |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |    exit_threshold_value      |            any number          |                    -Inf                     |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |      initial_solution        |               N/A              |                 randomly set                |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |         ising_qubo           |        'ising' or 'qubo'       |                    'ising'                  |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |          lp_solver           |               N/A              |   uses Coin-or Linear Programming solver    |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |  max_num_state_evaluations   |               >= 0             |                  50,000,000                 |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |         random_seed          |               >= 0             |                  randomly set               |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |           timeout            |               >= 0.0           |                      10.0                   |
%    +------------------------------+--------------------------------+---------------------------------------------+
%    |           verbose            |               [0 2]            |                       0                     |
%    +------------------------------+--------------------------------+---------------------------------------------+
%
%  Output
%    bestSolution: A column vector of +1/-1 values giving the best solution.
%    bestEnergy: best energy found for the given objective function.
%    info: a structure which has the following fields:
%       num_state_evaluations: number of state evaluations.
%       num_obj_func_calls: number of objective function calls.
%       num_solver_calls: number of solver (local/remote) calls.
%       num_lp_solver_calls: number of lp solver calls.
%       state_evaluations_time: state evaluations time (seconds).
%       solver_calls_time: solver (local/remote) calls time (seconds).
%       lp_solver_calls_time: lp solver calls time (seconds).
%       total_time: total running time of sapiSolveQSage (seconds).
%       history: n by 6 matrix showing number of state evaluations, number of objective function calls,
%                number of solver (local/remote) calls, number of lp solver calls, time (seconds) and objective value.
%
%  See also sapiSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if ~isstruct(solverParams) || ~isstruct(qsageParams)
    error('solverParams and qsageParams must be structures.');
end

isingSolver = struct;
solverProps = sapiSolverProperties(solver);
isingSolver.qubits = solverProps.qubits;
isingSolver.couplers = solverProps.couplers;
if isfield(solverProps, 'h_range')
    isingSolver.h_range = solverProps.h_range;
end
if isfield(solverProps, 'j_range')
    isingSolver.j_range = solverProps.j_range;
end
isingSolver.solve_ising = @(h, J)sapiSolveIsing(solver, h, J, solverParams);

[bestSolution bestEnergy info] = solve_qsage_mex(objectiveFunction, numVars, isingSolver, qsageParams);

end
