# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import random
from dwave_sapi2.util import reduce_degree


def increase_degree(terms, mapping):
    mapping = dict((a, (i, j)) for a, i, j in mapping)
    big_terms = []
    for term in terms:
        term = set(term)
        done = False
        while not done:
            done = True
            for v in tuple(term):
                if v in mapping:
                    term.remove(v)
                    term.update(mapping[v])
                    done = False
        big_terms.append(term)
    return big_terms


def random_terms(min_size, max_size, max_term_size):
    num_terms = random.randint(min_size, max_size)
    max_var = 2 * max_term_size + 1
    terms = []
    for _ in xrange(num_terms):
        term_size = random.randint(1, max_term_size)
        terms.append(random.sample(xrange(max_var), term_size))
    return terms


def pytest_generate_tests(metafunc):
    if 'terms' in metafunc.funcargnames:
        num_tests = 8
        min_terms = 5
        max_terms = 20
        max_term_size = 8
        test_data = [random_terms(min_terms, max_terms, max_term_size)
                     for _ in xrange(num_tests)]
        metafunc.parametrize('terms', test_data, ids=test_data)


def test(terms):
    small_terms, mapping = reduce_degree(terms)
    term_sets = map(set, terms)
    assert increase_degree(small_terms, mapping) == term_sets
