# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from array import array
from base64 import b64encode
from random import randint, sample
import pytest
import sapience


def make_randomsols():
    inactive = set(sample(range(sapience.C4_NUMQUBITS), randint(0, 128)))
    return [[3 if i in inactive else randint(0, 1) for i in
             range(sapience.C4_NUMQUBITS)] for _ in range(randint(1, 20))]


RANDOM_SOLS = [make_randomsols() for _ in range(10)]


def test_encode_sols_empty():
    assert sapience.encode_sols([]) == b''


def test_encode_sols_interesting():
    sols = [[1, 0, 1, 3, 0, 1, 0, 1, 3, 0, 3, 3, 1, 0, 1, 0, 1, 0, 1, 0, 1],
            [0, 1, 0, 3, 1, 0, 1, 0, 3, 1, 3, 3, 0, 1, 0, 1, 0, 1, 0, 1, 0],
            [0, 0, 0, 3, 0, 0, 0, 0, 3, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            [1, 1, 1, 3, 1, 1, 1, 1, 3, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1]]
    expected = b64encode(b'\xaa\xaa\x80\x55\x55\x00\x00\x00\x00\xff\xff\x80')
    assert sapience.encode_sols(sols) == expected


@pytest.mark.parametrize("qsols", RANDOM_SOLS,
                         ids=[','.join(''.join(str(i) for i in sol)
                                       for sol in sols)
                              for sols in RANDOM_SOLS])
def test_encode_sols_isingqubo(qsols):
    isols = [[-1 if i == 0 else i for i in sol] for sol in qsols]
    assert sapience.encode_sols(qsols) == sapience.encode_sols(isols)


def test_encode_answer_empty_raw():
    answer = {'energies': [], 'solutions': []}
    expected = {'format': 'qp', 'num_variables': 128, 'active_variables': b'',
                'energies': b'', 'solutions': b'', 'timing': {}}
    assert sapience.encode_answer(answer) == expected


def test_encode_answer_empty_hist():
    answer = {'energies': [], 'solutions': [], 'num_occurrences': []}
    expected = {'format': 'qp', 'num_variables': 128, 'active_variables': b'',
                'energies': b'', 'solutions': b'', 'num_occurrences': b'',
                'timing': {}}
    assert sapience.encode_answer(answer) == expected


def test_encode_answer_full():
    answer = {'energies': [-1.0, 0.0, 0.125],
              'solutions': [[0] * sapience.C4_NUMQUBITS,
                            [1] * sapience.C4_NUMQUBITS,
                            [0, 1] * (sapience.C4_NUMQUBITS // 2)],
              'num_occurrences': [1, 23, 456]}
    expected = {'format': 'qp', 'num_variables': 128,
                'active_variables':
                    b64encode(array('i',
                                    list(range(sapience.C4_NUMQUBITS))).tobytes()),
                'energies':
                    b64encode(array('d', answer['energies']).tobytes()),
                'solutions': b64encode(b''.join([b'\x00' * 16, b'\xff' * 16, b'\x55' * 16])),
                'num_occurrences':
                    b64encode(array('i',
                                    answer['num_occurrences']).tobytes()),
                'timing': {}}
    assert sapience.encode_answer(answer) == expected


def test_encode_answer_small_raw():
    sols = [[3] * 128, [3] * 128]
    active = [10, 20, 31, 42, 111]
    for i in active:
        sols[0][i] = 2 * (i % 2) - 1
        sols[1][i] = -sols[0][i]
    answer = {'energies': [-2.0, 1234.5], 'solutions': sols}
    expected = {'format': 'qp', 'num_variables': 128,
                'active_variables': b64encode(array('i', active).tobytes()),
                'solutions': b64encode(b'\x28\xd0'),
                'energies': b64encode(array('d', answer['energies'])),
                'timing': {}}
    assert sapience.encode_answer(answer) == expected
