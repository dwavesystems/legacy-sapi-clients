# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from dwave_sapi2.util import ising_to_qubo
from util import normalized_matrix


def test_trivial():
    q, offset = ising_to_qubo([], {})
    assert q == {}
    assert offset == 0


def test_no_zeros():
    q, offset = ising_to_qubo([0, 0, 0], {(0, 0): 0, (4, 5): 0})
    assert q == {}
    assert offset == 0


def test_j_diag():
    q, offset = ising_to_qubo([], {(0, 0): 1, (300, 300): 99})
    assert q == {}
    assert offset == 100


def test_typical():
    h = [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4]
    j = {(0, 0): -5, (0, 5): 2, (0, 8): 4, (1, 4): -5, (1, 7): 1, (2, 0): 5,
         (2, 1): 4, (3, 0): -1, (3, 6): -3, (3, 8): 3, (4, 0): 2, (4, 7): 3,
         (4, 9): 3, (5, 1): 3, (6, 5): -4, (6, 6): -5, (6, 7): -4, (7, 1): -4,
         (7, 8): 3, (8, 2): -4, (8, 3): -3, (8, 6): -5, (8, 7): -4, (9, 0): 4,
         (9, 1): -1, (9, 4): -5, (9, 7): 3}
    q, offset = ising_to_qubo(h, j)
    norm_q = normalized_matrix(q)
    assert norm_q == {(0, 0): -42, (0, 2): 20, (0, 3): -4, (0, 4): 8,
                      (0, 5): 8, (0, 8): 16, (0, 9): 16, (1, 1): -4,
                      (1, 2): 16, (1, 4): -20, (1, 5): 12, (1, 7): -12,
                      (1, 9): -4, (2, 2): -16, (2, 8): -16, (3, 3): 4,
                      (3, 6): -12, (4, 4): 2, (4, 7): 12, (4, 9): -8,
                      (5, 5): -2, (5, 6): -16, (6, 6): 34, (6, 7): -16,
                      (6, 8): -20, (7, 7): 8, (7, 8): -4, (7, 9): 12,
                      (8, 8): 18}
    assert offset == -8
