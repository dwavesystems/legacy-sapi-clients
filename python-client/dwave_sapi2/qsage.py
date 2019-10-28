# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.core import solve_ising
import qsage_impl


class _IsingSolver(object):
    def __init__(self, solver, params):
        self.solver = solver
        self.params = params
        self.qubits = solver.properties["qubits"]
        self.couplers = solver.properties["couplers"]
        if "h_range" in solver.properties:
            self.h_range = solver.properties["h_range"]
        if "j_range" in solver.properties:
            self.j_range = solver.properties["j_range"]

    def solve_ising(self, h, J):
        return solve_ising(self.solver, h, J, **self.params)


def solve_qsage(objfunc, num_vars, solver, solver_params, qsage_params):
    """
    answer = solve_qsage(objfunc, num_vars, solver, solver_params, qsage_params)
    (can be interrupted by Ctrl-C, will return the best solution found so far.)
    (for num_vars <= 10, the solve_qsage will do a brute-force search)

    solve_qsage tries to find the minimum of a pseudo-boolean function
    x = argmin_s f(s), s in {-1, 1}^N or {0, 1}^N where N is the number of variables.

    Args:
       objfunc: The implementation of the objective function f. solve_qsage assumes
                the following signature for f: y = f(s) where y is the returned
                objective value (y can be a list or tuple), s is a list of lists of states.

       num_vars: The number of variables.

       solver: solver that can solve ising problem. To be able to get the best performance,
               solver must be a sampler solver.

       solver_params: a dictionary of parameters for solver

       qsage_params: a dictionary of parameters for qsage algorithm, can use the following keys
                     (if the following parameter is set as None, it will use its default value):

              draw_sample: if set as False, solve_qsage will not draw samples, will only do tabu search
                           (must be a boolean, default = True)

              exit_threshold_value : if best value found by solve_qsage <= exit_threshold_value then exit
                                     (can be any number, default = float('-inf'))

              init_solution: the initial solution for the problem
                             (must be a list or tuple containing only -1/1 if "ising_qubo" parameter
                             is not set or set as "ising"; or 0/1 if "ising_qubo" parameter is
                             set as "qubo". The length of init_solution must be equal to num_vars,
                             default initial solution is randomly set)

              ising_qubo: "ising" or "qubo", if set as "ising", the return best solution will be -1/1;
                          if set as "qubo", the return best solution will be 0/1
                          (must be a string, default = "ising")

              lp_solver: a solver that can solve linear programming problems.
                     Finds the minimum of a problem specified by
                       min      f * x
                       st.      Aineq * x <= bineq
                                  Aeq * x = beq
                                  lb <= x <= ub

                     solve_qsage assumes the following signature for lp_solver:
                       x = solve(f, Aineq, bineq, Aeq, beq, lb, ub)

                       input arguments:
                         (suppose f's size is f_size, Aineq's size is Aineq_size by f_size, Aeq's size is Aeq_size by f_size)
                                     f: linear objective function, a list (size: f_size)
                                     Aineq: linear inequality constraints, a list of lists (size: Aineq_size by f_size)
                                     bineq: righthand side for linear inequality constraints, a list (size: Aineq_size)
                                     Aeq: linear equality constraints, a list of lists (size: Aeq_size by f_size)
                                     beq: righthand side for linear equality constraints, a list (size: Aeq_size)
                                     lb: lower bounds, a list (size: f_size)
                                     ub: upper bounds, a list (size: f_size)

                       output arguments:
                                     x: solution found by the optimization function (size: f_size)

                     (if not provided a default solver is used)

              max_num_state_evaluations: the maximum number of state evaluations, if
                                         the total number of state evaluations >= max_num_state_evaluations
                                         then exit
                                         (must be an integer >= 0, default = 50,000,000)

              random_seed: seed for random number generator that solve_qsage uses
                           (must be an integer >= 0, default is randomly set)

              timeout: timeout for solve_qsage (seconds)
                       (must be a number >= 0, default is approximately 10 seconds)

              verbose: control the output information
                   0: quiet
                   1, 2: different levels of verbosity
                   (must be an integer 0, or 1, or 2, default = 0)
                   when verbose is 1, the output information will be like:
                   [num_state_evaluations = ..., num_obj_func_calls = ..., num_solver_calls = ..., num_lp_solver_calls = ...],
                   best_energy = ..., distance to best_energy = ...
                   num_state_evaluations: the current total number of state evaluations
                   num_obj_func_calls: the current total number of objective function calls
                   num_solver_calls: the current total number of solver calls
                   num_lp_solver_calls: the current total number of lp solver calls
                   best_energy: the global best energy found so far
                   distance to best_energy: the Hamming distance between the global best state
                                            found so far and the current state found by tabu search
                   when verbose is 2, in addition to the output information when verbose is 1, the
                   following output information will also be shown:
                   sample_num = ...
                   min_energy = ...
                   move_length = ...
                   sample_num: the number of unique samples returned by sampler
                   min_energy: minimum energy found during the current phase of tabu search
                   move_length: the length of the move (the hamming distance between the current
                                state and the new state)

    Returns:
       answer is a dictionary which has the following keys:
          "best_solution": A list of +1/-1 or 1/0 values giving the best solution.
          "best_energy": best energy found for the given objective function.
          "num_state_evaluations": number of state evaluations.
          "num_obj_func_calls": number of objective function calls.
          "num_solver_calls": number of solver (local/remote) calls.
          "num_lp_solver_calls": number of lp solver calls.
          "state_evaluations_time": state evaluations time (seconds).
          "solver_calls_time": solver (local/remote) calls time (seconds).
          "lp_solver_calls_time": lp solver calls time (seconds).
          "total_time": total running time of solve_qsage (seconds).
          "history": A list of lists of number of state evaluations, number of objective function calls,
                     number of solver (local/remote) calls, number of lp solver calls, time (seconds)
                     and objective value.

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       RuntimeError: error occurred at run time
    """
    ising_solver = _IsingSolver(solver, solver_params)
    return qsage_impl.solve_qsage_impl(objfunc, num_vars, ising_solver, qsage_params)
