# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from array import array
from base64 import b64encode
from random import choice, gauss, randint, random, randrange, sample
from io import StringIO
import json
import pytest
import mock

import sapilocal
import sapience


def random_problemdata():
    activeq = set(sample(range(sapience.C4_NUMQUBITS),
                         randint(0, sapience.C4_NUMQUBITS)))
    activec = [c for c in sapience.C4_COUPLERS
               if activeq.issuperset(c)]
    lin = array('d', [float('nan')] * sapience.C4_NUMQUBITS)
    for i in activeq:
        lin[i] = gauss(0, 10)
    quad = array('d', [gauss(0, 10) for _ in activec])
    problem = {(i, i): lin[i] for i in activeq}
    problem.update(dict(list(zip(activec, quad))))
    encoded = {'format': 'qp', 'lin': b64encode(lin.tobytes()),
               'quad': b64encode(quad.tobytes())}
    return encoded, problem


def random_params(active_vars, include_beta):
    params = {
        'num_reads': randint(0, sapience.MAX_NUM_READS),
        'max_answers': randint(0, 1000000)
    }
    user_params = params.copy()
    answer_mode = choice(['raw', 'histogram', None])
    if answer_mode is not None:
        user_params['answer_mode'] = answer_mode
    params['answer_histogram'] = answer_mode != 'raw'
    if include_beta:
        user_params['beta'] = params['beta'] = randint(0, 10)

    params['num_vars'] = sapience.C4_NUMQUBITS
    active_vars = set(active_vars)
    params['var_order'] = [i for i in sapience.C4_VARORDER if i in active_vars]
    params['active_vars'] = params['var_order']
    params['active_var_pairs'] = tuple(c for c in sapience.C4_COUPLERS
                                       if active_vars.issuperset(c))
    return user_params, params


def random_problem(include_beta):
    data, sldata = random_problemdata()
    active_vars = {i for p in sldata for i in p}
    params, slparams = random_params(active_vars, include_beta)

    problem = {'type': choice(['ising', 'qubo']),
               'data': data, 'params': params}
    slproblem = {'type': problem['type'], 'data': sldata, 'params': slparams}
    return problem, slproblem


def random_sols(num_sols):
    active_vars = array('i', sorted(sample(range(sapience.C4_NUMQUBITS),
                                           randint(0, sapience.C4_NUMQUBITS))))
    if num_sols == 0 or not active_vars:
        return b'', [], array('i')

    sol_len = len(active_vars)
    mask = 255 - (255 >> 8 + sol_len % -8)
    solsize = (sol_len + 7) // 8
    ising = random() < 0.5
    sols = []
    slsols = []
    for _ in range(num_sols):
        solbytes = array('B', [randrange(256) for _ in range(solsize)])
        solbytes[-1] &= mask
        sols.append(solbytes.tobytes())
        solarray0 = [(b >> (7 - i)) & 1 for b in solbytes for i in range(8)]
        if ising:
            solarray0 = [2 * x - 1 for x in solarray0]
        solarray = [3] * sapience.C4_NUMQUBITS
        for i, s in zip(active_vars, solarray0):
            solarray[i] = s
        slsols.append(solarray)
    return b64encode(b''.join(sols)), slsols, active_vars


def random_answer():
    num_sols = randint(0, 100)
    energies = array('d', [gauss(0, 10) for _ in range(num_sols)])
    if choice([True, False]):
        num_occ = array('i', [randint(1, 1000) for _ in range(num_sols)])
    else:
        num_occ = None
    solutions, slsolutions, active_vars = random_sols(num_sols)
    answer = {
        'format': 'qp',
        'num_variables': sapience.C4_NUMQUBITS,
        'active_variables': b64encode(active_vars.tobytes()),
        'energies': b64encode(energies),
        'solutions': solutions,
        'timing': {}
    }
    slanswer = {
        'energies': energies.tolist(),
        'solutions': slsolutions
    }
    if num_occ is not None:
        answer['num_occurrences'] = b64encode(num_occ.tobytes())
        slanswer['num_occurrences'] = num_occ.tolist()
    return answer, slanswer


@pytest.mark.parametrize("problem,slproblem,exp_answer,slanswer",
                         [random_problem(False) + random_answer()
                          for _ in range(100)])
def test_solve_optimize(problem, slproblem, exp_answer, slanswer, monkeypatch):
    answer = None
    try:
        mock_solve = mock.Mock(spec=sapilocal.orang_optimize,
                               return_value=slanswer)
        monkeypatch.setitem(sapience.SOLVERS, 'c4-optimize',
                            (mock_solve,) + sapience.SOLVERS['c4-optimize'][1:])
        answer = sapience.solve('c4-optimize', problem)
        assert answer == exp_answer
        mock_solve.assert_called_once_with(slproblem['type'],
                                           slproblem['data'],
                                           slproblem['params'])
    except AssertionError:
#        print problem
#        print slproblem
#        print exp_answer
#        print slanswer
#        print answer
        raise


@pytest.mark.parametrize("problem,slproblem,exp_answer,slanswer",
                         [random_problem(True) + random_answer()
                          for _ in range(100)])
def test_solve_sample(problem, slproblem, exp_answer, slanswer, monkeypatch):
    answer = None
    try:
        mock_solve = mock.Mock(spec=sapilocal.orang_optimize,
                               return_value=slanswer)
        monkeypatch.setitem(sapience.SOLVERS, 'c4-sample',
                            (mock_solve,) + sapience.SOLVERS['c4-sample'][1:])
        answer = sapience.solve('c4-sample', problem)
        if answer != exp_answer:
            print("ANS:", answer)
            print("EANS:", exp_answer)
        assert answer == exp_answer
        mock_solve.assert_called_once_with(slproblem['type'],
                                           slproblem['data'],
                                           slproblem['params'])
    except AssertionError:
#        print problem
#        print slproblem
#        print exp_answer
#        print slanswer
#        print answer
        raise
