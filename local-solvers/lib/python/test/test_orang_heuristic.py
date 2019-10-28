# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import pytest
import sapilocal

VALID_PARAMS = {
    'iteration_limit': 10,
    'max_bit_flip_prob': 1.0 / 8.0,
    'max_local_complexity': 9,
    'min_bit_flip_prob': 1.0 / 32.0,
    'local_stuck_limit': 8,
    'num_perturbed_copies': 4,
    'num_variables': 0,
    'time_limit_seconds': 5.0
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
    sapilocal.orang_heuristic('ising', {}, VALID_PARAMS)
    sapilocal.orang_heuristic('qubo', {}, VALID_PARAMS)

def test_bad_problem_type():
    with pytest.raises(ValueError):
        sapilocal.orang_heuristic('zzz', {}, VALID_PARAMS)

def test_bad_problem():
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', '2 + 2 = ?', VALID_PARAMS)
   
def test_bad_params():
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('iteration_limit'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(iteration_limit="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(iteration_limit=-1))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('max_bit_flip_prob'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(max_bit_flip_prob="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(max_bit_flip_prob=-0.1))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('max_local_complexity'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(max_local_complexity="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(max_local_complexity=-1))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('min_bit_flip_prob'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(min_bit_flip_prob="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(min_bit_flip_prob=-0.1))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(min_bit_flip_prob=0.8, max_bit_flip_prob=0.6))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('local_stuck_limit'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(local_stuck_limit="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(local_stuck_limit=-1))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('num_perturbed_copies'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(num_perturbed_copies="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(num_perturbed_copies=-1))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('num_variables'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(num_variables="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(num_variables=-1))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(random_seed="a"))
    with pytest.raises(KeyError):
        sapilocal.orang_heuristic('ising', {}, _missing('time_limit_seconds'))
    with pytest.raises(TypeError):
        sapilocal.orang_heuristic('ising', {}, _params(time_limit_seconds="a"))
    with pytest.raises(RuntimeError):
        sapilocal.orang_heuristic('ising', {}, _params(time_limit_seconds=-1.0))
