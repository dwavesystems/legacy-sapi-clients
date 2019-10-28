# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import random
import math
from itertools import product
import pytest
from dwave_sapi2.util import make_quadratic


def random_f(num_vars):
    f = [random.randint(-100, 100) for _ in xrange(2 ** num_vars)]
    f[0] = 0
    return f


def pytest_generate_tests(metafunc):
    if 'f' in metafunc.funcargnames:
        num_tests = 8
        min_vars = 1
        max_vars = 5
        test_data = [random_f(random.randint(min_vars, max_vars))
                     for _ in xrange(num_tests)]
        metafunc.parametrize('f', test_data, ids=test_data)


def eval_q(q, x):
    return sum(v * x[i] * x[j] for (i, j), v in q.iteritems())


def q_vals(q, num_vars):
    f = []
    total_vars = 1 + max(max(k) for k in q)
    a_vars = total_vars - num_vars
    for x in product(*[[0, 1]] * num_vars):
        f.append(min(eval_q(q, x[::-1] + a)
                     for a in product(*[[0, 1]] * a_vars)))
    return f


def test_bad_f_size():
    with pytest.raises(ValueError):
        make_quadratic([])
    with pytest.raises(ValueError):
        make_quadratic([0, 1, 1])


def test_bad_f_zero():
    with pytest.raises(ValueError):
        make_quadratic([1])


def test_trivial():
    assert make_quadratic([0]) == ({}, [], [])


def test(f):
    q, new_terms, mapping = make_quadratic(f)
    assert q_vals(q, int(math.log(len(f), 2))) == f
    assert all(len(t) <= 2 for t in new_terms)
