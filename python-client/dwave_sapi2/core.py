# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

import datetime
import warnings
from itertools import chain
from . import remote, local

_NOVALIDATION_MESSAGE = """unable to validate parameter names

There is no information available about the parameter names that this
solver accepts. It still works but misspelled parameter names will be
silently ignored. Please complain to your system administrator.

*** This is not an error! Your problem has been submitted! ***
"""


def _ising_problem(h, j):
    return dict(chain(
        (((i, i), e) for i, e in enumerate(h) if e != 0),
        (((i, j), e) for (i, j), e in j.iteritems() if e != 0)
    ))


def _check_j_diagonal(j):
    for (i, j), e in j.iteritems():
        if i == j:
            raise ValueError("j's diagonal entries must be zero.")


def _check_solver_params(solver, params):
    if 'parameters' not in solver.properties:
        warnings.warn(_NOVALIDATION_MESSAGE, stacklevel=3)
    else:
        paramprop = solver.properties['parameters']
        for param in params:
            if param not in paramprop and not param.startswith('x_'):
                raise ValueError('"{}" is not a valid parameter for this '
                                 'solver'.format(param))


def solve_ising(solver, h, j, **params):
    """
    answer = solve_ising(solver, h, j, **params)

    Args:
       solver: solver object that can solve ising problem.

       h: h value for an ising problem, can be a list or a tuple.

       j: J value for an ising problem, must be a dict.

       **params: keyword parameters for solver.

    Returns:
       answer:
       A dict which has the following keys:
          "solutions": A list of lists
          "energies": A list
          "num_occurrences": A list (only appears in histogram mode)
          "timing": A dict (only appears if the solver is a hardware solver)

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       KeyboardInterrupt: when Ctrl-C is pressed
       RuntimeError: error occurred at run time
    """
    _check_j_diagonal(j)
    _check_solver_params(solver, params)
    return _solve(solver, 'ising', _ising_problem(h, j), params)


def solve_qubo(solver, q, **params):
    """
    answer = solve_qubo(solver, q, **params)

    Args:
       solver: solver object that can solve qubo problem.

       q: Q value for a qubo problem, must be a dict.

       **params: keyword parameters for solver.

    Returns:
       answer:
       A dict which has the following keys:
          "solutions": A list of lists
          "energies": A list
          "num_occurrences": A list (only appears in histogram mode)
          "timing": A dict (only appears if the solver is a hardware solver)

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       KeyboardInterrupt: when Ctrl-C is pressed
       RuntimeError: error occurred at run time
    """
    _check_solver_params(solver, params)
    return _solve(solver, 'qubo', q, params)


def async_solve_ising(solver, h, j, **params):
    """
    submitted_problem = async_solve_ising(solver, h, j, **params)

    Args:
       solver: solver object that can solve ising problem.

       h: h value for an ising problem, can be a list or a tuple.

       j: J value for an ising problem, must be a dict.

       **params: keyword parameters for solver.

    Returns:
       submitted_problem: a submitted problem that have done() and
                          result() methods

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       KeyboardInterrupt: when Ctrl-C is pressed
       RuntimeError: error occurred at run time
    """
    _check_j_diagonal(j)
    _check_solver_params(solver, params)
    return solver.submit('ising', _ising_problem(h, j), params)


def async_solve_qubo(solver, q, **params):
    """
    submitted_problem = async_solve_qubo(solver, q, **params)

    Args:
       solver: solver object that can solve ising problem.

       q: Q value for a qubo problem, must be a dict.

       **params: keyword parameters for solver.

    Returns:
       submitted_problem: a submitted problem that have done() and
                          result() methods

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       KeyboardInterrupt: when Ctrl-C is pressed
       RuntimeError: error occurred at run time
    """
    _check_solver_params(solver, params)
    return solver.submit('qubo', q, params)


def _endtime(timeout):
    starttime = datetime.datetime.now()
    timeout = max(timeout, 0)
    endtime = datetime.datetime.max
    if (endtime - starttime).total_seconds() > timeout:
        endtime = starttime + datetime.timedelta(seconds=timeout)
    return endtime


def await_completion(submitted_problems, min_done, timeout):
    """
    done = await_completion(submitted_problems, min_done, timeout)

    Args:
       submitted_problems: a list of asynchronous problems
                           (returned by async_solve_ising/async_solve_qubo)

       min_done: minimum number of problems that must be completed before
                returning (without timeout).

       timeout: maximum time to wait (in seconds)

    Returns:
       done: True if returning because enough problems completed, False if
             returning because of timeout
    """

    min_done = min(min_done, len(submitted_problems))
    remote_submitted_problems = []
    for sp in submitted_problems:
        if isinstance(sp, remote._RemoteSubmittedProblem):
            remote_submitted_problems.append(sp)
        elif not isinstance(sp, local._LocalSubmittedProblem):
            raise TypeError("invalid asynchronous problem")

    new_min_done = max(0, min_done - (len(submitted_problems) -
                                      len(remote_submitted_problems)))
    if new_min_done == 0:
        return True
    return remote._await_remote_completion(remote_submitted_problems,
                                           new_min_done, _endtime(timeout))


def _solve(solver, problem_type, problem, params):
    sp = solver.submit(problem_type, problem, params)
    if not await_completion([sp], 1, float('inf')):
        raise RuntimeError("problem not done")
    return sp.result()
