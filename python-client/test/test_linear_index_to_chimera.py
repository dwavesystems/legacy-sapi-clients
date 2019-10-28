# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from dwave_sapi2.util import linear_index_to_chimera


def test_trivial():
    assert linear_index_to_chimera([], 1) == []


def test_full_small():
    m = 4
    n = 3
    t = 2
    expected = [[r, c, u, k]
                for r in xrange(m)
                for c in xrange(n)
                for u in xrange(2)
                for k in xrange(t)]
    assert linear_index_to_chimera(range(2 * m * n * t), m, n, t) == expected


def test_larger():
    li = [214, 370, 1210, 366, 1172, 350, 1338, 503, 283, 361, 887, 681]
    expected = [[2, 1, 1, 4], [3, 6, 1, 4], [12, 4, 1, 4], [3, 6, 1, 0],
                [12, 1, 1, 2], [3, 5, 0, 2], [13, 7, 1, 0], [5, 1, 1, 5],
                [2, 7, 1, 1], [3, 6, 0, 1], [9, 1, 1, 5], [7, 0, 1, 3]]
    assert linear_index_to_chimera(li, 15, 8, 6) == expected
