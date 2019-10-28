# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import pytest
import sapilocal

VALID_PARAMS = {
    'num_vars': 0,
    'active_vars': [],
    'active_var_pairs': [],
    'var_order': [],
    'num_reads': 1,
    'max_answers': 1,
    'answer_histogram': True,
}


def _params(**kwargs):
    d = dict(VALID_PARAMS)
    d.update(**kwargs)
    return d


def _missing(key):
    d = dict(VALID_PARAMS)
    del d[key]
    return d


def test_valid():
    sapilocal.orang_optimize('ising', {}, VALID_PARAMS)
    sapilocal.orang_optimize('qubo', {}, VALID_PARAMS)


def test_ising_hist():
    params = {
        'num_vars': 8,
        'active_vars': [0, 2, 4, 6],
        'active_var_pairs': [(0, 2), (2, 4), (4, 6)],
        'var_order': (4, 2, 6, 0),
        'num_reads': 10000,
        'max_answers': 3,
        'answer_histogram': True,
    }
    problem = {(0, 0): 0.25, (0, 2): -1, (2, 4): 1, (4, 6): -0.5}
    result = sapilocal.orang_optimize('ising', problem, params)

    assert result == {
        'energies': [-2.75, -2.25, -1.75],
        'solutions': [[-1, 3, -1, 3, +1, 3, +1, 3],
                      [+1, 3, +1, 3, -1, 3, -1, 3],
                      [-1, 3, -1, 3, +1, 3, -1, 3]],
        'num_occurrences': [9998, 1, 1]
    }


def test_ising_no_hist():
    params = {
        'num_vars': 5,
        'active_vars': range(5),
        'active_var_pairs': ((0, 2), (1, 2), (3, 2), (4, 2)),
        'var_order': (4, 3, 0, 1, 2),
        'num_reads': 6,
        'max_answers': 6,
        'answer_histogram': False,
    }
    problem = {(0, 2): -0.5, (1, 2): -0.5, (2, 4): -0.5}
    result = sapilocal.orang_optimize('ising', problem, params)

    assert result == {
        'energies': [-1.5, -1.5, -0.5, -0.5, -0.5, -0.5],
        'solutions': [[-1, -1, -1, -1, -1],
                      [+1, +1, +1, -1, +1],
                      [-1, -1, -1, -1, +1],
                      [-1, +1, -1, -1, -1],
                      [-1, +1, +1, -1, +1],
                      [+1, -1, -1, -1, -1]]
    }


def test_qubo_hist():
    params = {
        'num_vars': 10,
        'active_vars': range(9),
        'active_var_pairs': [(i, i+1) for i in range(8)],
        'var_order': list(reversed(range(9))),
        'num_reads': 8888,
        'max_answers': 4,
        'answer_histogram': True,
    }
    problem = {(0, 0): -10, (0, 1): 3, (1, 1): 2, (1, 2): 6, (2, 2): -4,
               (2, 3): -1, (3, 4): -1, (4, 5): -1, (5, 6): -1, (6, 7): -1,
               (7, 8): -1}
    result = sapilocal.orang_optimize('qubo', problem, params)

    assert result == {
        'energies': [-20, -19, -18, -18],
        'solutions': [[1, 0, 1, 1, 1, 1, 1, 1, 1, 3],
                      [1, 0, 1, 1, 1, 1, 1, 1, 0, 3],
                      [1, 0, 1, 0, 1, 1, 1, 1, 1, 3],
                      [1, 0, 1, 1, 0, 1, 1, 1, 1, 3]],
        'num_occurrences': [8885, 1, 1, 1]
    }


def test_qubo_no_hist():
    params = {
        'num_vars': 4,
        'active_vars': range(4),
        'active_var_pairs': [[0, 1], [0, 2], [0, 3], [1, 2], [1, 3], [2, 3]],
        'var_order': range(4),
        'num_reads': 7,
        'max_answers': 7,
        'answer_histogram': False,
    }
    problem = {(0, 0): -1, (1, 1): -1, (2, 2): -1, (3, 3): -1,
               (0, 1): 0.25, (0, 2): 0.25, (0, 3): 0.25,
               (1, 2): 0.5, (1, 3): 0.5, (2, 3): -1}
    result = sapilocal.orang_optimize('qubo', problem, params)

    assert result == {
        'energies': [-3.50, -3.25, -3.00, -3.00, -2.00, -2.00, -1.75],
        'solutions': [[1, 0, 1, 1],
                      [1, 1, 1, 1],
                      [0, 0, 1, 1],
                      [0, 1, 1, 1],
                      [1, 1, 0, 1],
                      [1, 1, 1, 0],
                      [1, 0, 0, 1]]
    }


def test_bad_problem_type():
    with pytest.raises(ValueError):
        sapilocal.orang_optimize('zzz', {}, VALID_PARAMS)


def test_bad_problem():
    params = dict(VALID_PARAMS)
    params.update(num_vars=4,
                  active_vars=(0, 1, 2, 3),
                  active_var_pairs=((0, 1), (1, 2), (2, 3), (3, 4)),
                  var_order=(0, 1, 2, 3))

    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', '2 + 2 = ?', params)
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {(0, 2): -1}, params)


def test_bad_params():
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('num_vars'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(num_vars='lots'))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(num_vars=-1))
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('active_vars'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(active_vars='all'))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(active_vars=[0]))
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('active_var_pairs'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(active_var_pairs=6))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(active_var_pairs=[(1, 2)]))
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('var_order'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(var_order='all'))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(var_order=[0]))
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('num_reads'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(num_reads=[1, 2, 3]))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(num_reads=-2))
    with pytest.raises(KeyError):
        sapilocal.orang_optimize('ising', {}, _missing('max_answers'))
    with pytest.raises(TypeError):
        sapilocal.orang_optimize('ising', {}, _params(max_answers=[1, 2, 3]))
    with pytest.raises(RuntimeError):
        sapilocal.orang_optimize('ising', {}, _params(max_answers=-2))

