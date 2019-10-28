% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

% SOLVE_QSAGE_MEX solve a QSage problem.
%
%  [bestSolution bestEnergy info] = solve_qsage_mex(isingSolver, objectiveFunction, numVars, params)
%  (can be interrupted by Ctrl-C, will return the best solution found so far.)
%  (for numVars <= 10, the solve_qsage_mex will do a brute-force search)
%
%  Input parameters
%    objectiveFunction: the implementation of the objective function f.
%      solve_qsage_mex assumes the following signature:
%      y = f(states) where states is a two-dimensional matrix. Each column
%      in states is a state. y is the returned objective value for each state.
%
%    numVars: the number of variables.
%    params: structure of solver parameters. Must be a structure.
%
%    isingSolver: a handle to the ising solver that is used to solve ising problems.
%                 The isingSolver must be a struct which has the following
%                 fields:
%                   isingSolver = struct;
%                   isingSolver.qubits: a matrix containing qubits
%                   isingSolver.couplers: a 2 by n matrix containing couplers
%                   isingSolver.h_range: [**optional**] a 1 by 2 matrix
%                   isingSolver.j_range: [**optional**] a 1 by 2 matrix
%                   containing [h_min h_max J_min J_max]
%                   isingSolver.solve_ising: a function handle which solves
%                   ising problems, the input parameter is h and J,
%                   the return value of isingSolver.solve_ising
%                   must be a struct which has the following fields:
%                      solutions: each column is a solution
%                      energies: a matrix containing energies
%                      num_occurrences: [**optional**] a matrix containing
%                      number of occurrences
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
%       total_time: total running time of solve_qsage_mex (seconds).
%       history: n by 6 matrix showing number of state evaluations, number of objective function calls,
%                number of solver (local/remote) calls, number of lp solver calls, time (seconds) and objective value.
%
%   parameters for solve_qsage_mex:
%
%   draw_sample: if false, solve_qsage_mex will not draw samples, will only do tabu search
%                (must be a boolean, default = true)
%
%   exit_threshold_value : if best value found by solve_qsage_mex <= exit_threshold_value then exit
%                          (can be any number, default = -Inf)
%
%   init_solution: the initial solution for the problem
%                  (must be a matrix containing only -1/1 if "ising_qubo" parameter is not set or set as "ising";
%                  or 0/1 if "ising_qubo" parameter is set as "qubo". The number of elements of init_solution
%                  must be equal to numVars, default initial solution is randomly set)
%
%   ising_qubo: "ising" or "qubo", if set as "ising", the return best solution will be -1/1;
%               if set as "qubo", the return best solution will be 0/1
%               (must be a string, default = "ising")
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
%              solve_qsage_mex assumes the following signature for lp_solver:
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
%   random_seed: seed for random number generator that solve_qsage_mex uses
%                (must be an integer >= 0, default is randomly set)
%
%   timeout: timeout for solve_qsage_mex (seconds, must be a number >= 0, default is approximately 10 seconds)
%
%   verbose: control the output information
%            0: quiet
%            1, 2: different levels of verbosity
%            (must be an integer 0, or 1, or 2, default = 0)
%                 when verbose is 1, the output information will be like:
%                 [num_state_evaluations = ..., num_obj_func_calls = ..., num_solver_calls = ..., num_lp_solver_calls = ...],
%                 best_energy = ..., distance to best_energy = ...
%                 num_state_evaluations: the current total number of state evaluations
%                 num_obj_func_calls: the current total number of objective function calls
%                 num_solver_calls: the current total number of solver calls
%                 num_lp_solver_calls: the current total number of lp solver calls
%                 best_energy: the global best energy found so far
%                 distance to best_energy: the hamming distance between the global best state found so far
%                                          and the current state found by tabu search
%                 when verbose is 2, in addition to the output information when verbose is 1, the following
%                 output information will also be shown:
%                 sample_num = ...
%                 min_energy = ...
%                 move_length = ...
%                 sample_num: the number of unique samples returned by sampler
%                 min_energy: minimum energy found during the current phase of tabu search
%                 move_length: the length of the move (the hamming distance between the current state and
%                              the new state)
%

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
