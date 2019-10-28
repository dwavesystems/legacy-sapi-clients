# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from dwave_sapi2.util import qubo_to_ising
from util import normalized_matrix


def test_trivial():
    h, j, offset = qubo_to_ising({})
    assert h == []
    assert j == {}
    assert offset == 0


def test_no_zeros():
    h, j, offset = qubo_to_ising({(0, 0): 0, (4, 5): 0})
    assert h == []
    assert j == {}
    assert offset == 0


def test_typical():
    q = {(0, 0): 4, (0, 3): 5, (0, 5): 4, (1, 1): 5, (1, 6): 1, (1, 7): -2,
         (1, 9): -3, (3, 0): -2, (3, 1): 2, (4, 5): 4, (4, 8): 2, (4, 9): -1,
         (5, 1): 2, (5, 6): -5, (5, 8): -4, (6, 0): 1, (6, 5): 2, (6, 6): -4,
         (6, 7): -2, (7, 0): -2, (7, 5): -3, (7, 6): -5, (7, 7): -3, (7, 8): 1,
         (8, 0): 2, (8, 5): 1, (9, 7): -3}
    h, j, offset = qubo_to_ising(q)
    assert h == [4, 2.5, 0, 1.25, 1.25, 0.25, -4, -5.5, 0.5, -1.75]
    norm_j = normalized_matrix(j)
    assert norm_j == {(0, 3): 0.75, (0, 5): 1, (0, 6): 0.25, (0, 7): -0.5,
                      (0, 8): 0.5, (1, 3): 0.5, (1, 5): 0.5, (1, 6): 0.25,
                      (1, 7): -0.5, (1, 9): -0.75, (4, 5): 1, (4, 8): 0.5,
                      (4, 9): -0.25, (5, 6): -0.75, (5, 7): -0.75,
                      (5, 8): -0.75, (6, 7): -1.75, (7, 8): 0.25,
                      (7, 9): -0.75}
    assert offset == -0.25
