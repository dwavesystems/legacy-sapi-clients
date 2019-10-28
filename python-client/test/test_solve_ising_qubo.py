# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from flexmock import flexmock
from dwave_sapi2.core import (async_solve_ising, solve_ising, async_solve_qubo, solve_qubo)


def test_async_solve_ising():
    h = [0, 1, -1, 0, 1]
    j = {(0, 1): -1, (1, 2): 1, (2, 4): -1}
    problem = {(0, 1): -1, (1, 1): 1, (1, 2): 1, (2, 2): -1, (2, 4): -1, (4, 4): 1}
    params = {'param': 'yes', 'another': 'sure'}
    ret = 'submitted-problem'

    solver = flexmock(properties={})
    solver.should_receive('submit').with_args('ising', problem, params).and_return(ret)

    assert async_solve_ising(solver, h, j, param='yes', another='sure') == ret


def test_async_solve_qubo():
    q = {(1, 1): -1, (10, 1): 2, (1, 100): -3}
    params = {'param': 'yes', 'another': 'sure'}
    ret = 'submitted-problem'

    solver = flexmock(properties={})
    solver.should_receive('submit').with_args('qubo', q, params).and_return(ret)

    assert async_solve_qubo(solver, q, param='yes', another='sure') == ret
