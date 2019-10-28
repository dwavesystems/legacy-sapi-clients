# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from dwave_sapi2.util import chimera_to_linear_index


def test_trivial():
    assert chimera_to_linear_index([], 1, 1, 1) == []
    assert chimera_to_linear_index([], [], [], [], 1, 1, 1) == []


def test_full_small():
    m, n, t = 2, 3, 4
    cr = []
    cc = []
    cu = []
    ck = []
    ci = []
    for r in xrange(m):
        for c in xrange(n):
            for u in xrange(2):
                for k in xrange(t):
                    cr.append(r)
                    cc.append(c)
                    cu.append(u)
                    ck.append(k)
                    ci.append((r, c, u, k))
    assert chimera_to_linear_index(ci, m, n, t) == range(2 * m * n * t)
    assert (chimera_to_linear_index(cr, cc, cu, ck, m, n, t)
            == range(2 * m * n * t))


def test_larger():
    m, n, t = 10, 13, 5
    cr = [9, 9, 1, 9, 9, 4, 8, 1, 4, 9, 7, 9]
    cc = [8, 0, 11, 12, 8, 9, 9, 5, 8, 2, 9, 0]
    cu = [0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1]
    ck = [4, 1, 2, 1, 3, 1, 2, 3, 4, 4, 2, 0]
    assert (chimera_to_linear_index(cr, cc, cu, ck, m, n, t) ==
            [1254, 1171, 242, 1296, 1258, 611,
             1137, 183, 604, 1194, 1007, 1175])
