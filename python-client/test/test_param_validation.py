# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import warnings

import pytest
from flexmock import flexmock

from dwave_sapi2.core import (solve_ising, solve_qubo, async_solve_ising,
                              async_solve_qubo)
from dwave_sapi2.local import _LocalSubmittedProblem


def test_no_parameters():
    def solve(*_):
        return None
    local_solver = flexmock()
    local_solver.should_receive('solve').replace_with(solve)
    async_result = _LocalSubmittedProblem(local_solver, None, None, None)
    solver = flexmock(properties={'blah': 123})
    solver.should_receive('submit').and_return(async_result).times(4)

    with warnings.catch_warnings(record=True) as w:
        warnings.simplefilter('always')
        solve_ising(solver, [], {})
        solve_qubo(solver, {})
        async_solve_ising(solver, [], {})
        async_solve_qubo(solver, {})

        assert len(w) == 4
        assert 'unable to validate parameter names' in str(w[0].message)
        assert 'unable to validate parameter names' in str(w[1].message)
        assert 'unable to validate parameter names' in str(w[2].message)
        assert 'unable to validate parameter names' in str(w[3].message)


def test_invalid_parameters():
    solver = flexmock(properties={'parameters': {'valid_param': None}})

    with pytest.raises(ValueError):
        solve_ising(solver, [], {}, invalid_param=123)
    with pytest.raises(ValueError):
        solve_qubo(solver, {}, invalid_param=123)
    with pytest.raises(ValueError):
        async_solve_ising(solver, [], {}, invalid_param=123)
    with pytest.raises(ValueError):
        async_solve_qubo(solver, {}, invalid_param=123)


def test_valid_parameters():
    def solve(*_):
        return 'the answer'
    local_solver = flexmock()
    local_solver.should_receive('solve').replace_with(solve)
    async_result = _LocalSubmittedProblem(local_solver, None, None, None)
    solver = flexmock(properties={'parameters': {'valid_param': None}})
    solver.should_receive('submit').and_return(async_result).times(4)

    assert solve_ising(solver, [], {}, valid_param=123) == 'the answer'
    assert solve_qubo(solver, {}, valid_param=123) == 'the answer'

    r = async_solve_ising(solver, [], {}, valid_param=123)
    assert r.result() == 'the answer'

    r = async_solve_qubo(solver, {}, valid_param=123)
    assert r.result() == 'the answer'


def test_ignore_x_parameters():
    def solve(*_):
        return 'the answer'
    local_solver = flexmock()
    local_solver.should_receive('solve').replace_with(solve)
    async_result = _LocalSubmittedProblem(local_solver, None, None, None)
    solver = flexmock(properties={'parameters': {'valid_param': None}})
    solver.should_receive('submit').and_return(async_result).times(4)

    assert solve_ising(solver, [], {}, x_hello=1) == 'the answer'
    assert solve_qubo(solver, {}, valid_param=123, x_a=2) == 'the answer'

    r = async_solve_ising(solver, [], {}, valid_param=123, x_blah='hello',
                          x_blarg='woooooo')
    assert r.result() == 'the answer'

    r = async_solve_qubo(solver, {}, x_abc=456, x_qaz=True,
                         x_qwerty=[1, 2, None], x_x_x_x='xxxx')
    assert r.result() == 'the answer'
