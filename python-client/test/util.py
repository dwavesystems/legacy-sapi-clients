# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from itertools import groupby


def normalized_matrix(mat):
    def key_fn(x):
        return x[0]

    smat = sorted(((sorted(k), v) for k, v in mat.iteritems()), key=key_fn)
    return dict((tuple(k), s) for k, g in groupby(smat, key=key_fn) for s in
                [sum(v for _, v in g)] if s != 0)
