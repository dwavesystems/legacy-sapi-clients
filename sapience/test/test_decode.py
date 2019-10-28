# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from array import array
from random import sample, randint
import pytest
import sapience

actives = [set(sample(range(sapience.C4_NUMQUBITS), randint(1, 100)))
           for _ in range(50)]


def test_decode_empty():
    lin = array('d', [float('nan')] * sapience.C4_NUMQUBITS)
    problem, activeq, activec = sapience.decode_c4_problem(lin.tobytes(), b'')
    assert problem == {}
    assert not activeq
    assert not activec


def test_decode_full():
    lin = array('d', list(range(128)))
    quad = array('d', list(range(-1, -1 - len(sapience.C4_COUPLERS), -1)))
    problem, activeq, activec = sapience.decode_c4_problem(lin.tobytes(), quad.tobytes())
    expected_problem = {(i, i): v for i, v in enumerate(lin)}
    expected_problem.update({k: v for k, v in zip(sapience.C4_COUPLERS, quad)})
    assert problem == expected_problem
    assert activeq == set(range(128))
    assert set(activec) == set(sapience.C4_COUPLERS)


@pytest.mark.parametrize('active_qubits', actives,
                         ids=[','.join(str(a) for a in aa) for aa in actives])
def test_decode_partial(active_qubits):
    active_couplers = [(i, j) for i, j in sapience.C4_COUPLERS if
                       i in active_qubits and j in active_qubits]
    problem = {(i, i): randint(-99, 99) for i in active_qubits}
    problem.update({k: randint(-99, 99) for k in active_couplers})
    lin = array('d', [float('nan')] * sapience.C4_NUMQUBITS)
    for i in active_qubits:
        lin[i] = problem[i, i]
    quad = array('d', (problem[k] for k in active_couplers))
    new_problem, new_activeq, new_activec = sapience.decode_c4_problem(
        lin.tobytes(), quad.tobytes())
    assert new_problem == problem
    assert new_activeq == set(active_qubits)
    assert set(new_activec) == set(active_couplers)


@pytest.mark.parametrize("linsize,linval,quadsize",
                         [(0, 0.0, 0),
                          (sapience.C4_NUMQUBITS - 1, 0.0, 0),
                          (sapience.C4_NUMQUBITS, 0.0, 0),
                          (sapience.C4_NUMQUBITS, float('nan'),
                           len(sapience.C4_COUPLERS))])
def test_decode_invalid_size(linsize, linval, quadsize):
    linbytes = array('d', [float(linval) for _ in range(linsize)]).tobytes()
    quadbytes = array('d', [0] * quadsize).tobytes()
    with pytest.raises(ValueError):
        sapience.decode_c4_problem(linbytes, quadbytes)
