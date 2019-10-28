# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from dwave_sapi2.embedding import unembed_answer


def test_trivial():
    assert unembed_answer([], []) == []
    assert unembed_answer([], [], 'minimize_energy') == []
    assert unembed_answer([], [], 'vote') == []
    assert unembed_answer([], [], 'discard') == []
    assert unembed_answer([], [], 'weighted_random') == []


def test_discard():
    embeddings = ({2, 5, 6}, [0], (1, 3))
    solutions = [
        [+1, +1, +1, +1, +3, +1, +1],
        [+1, +1, -1, +1, +1, -1, +1],
        [+1, +1, -1, +1, +1, -1, -1],
        [+1, -1, -1, +1, +1, -1, -1],
        [+1, -1, -1, -1, +1, -1, -1],
        [-1, -1, -1, -1, 33, -1, -1]
    ]
    expected = [
        [+1, +1, +1],
        [-1, +1, +1],
        [-1, +1, -1],
        [-1, -1, -1]
    ]
    assert unembed_answer(solutions, embeddings, 'discard') == expected


def test_discard_all():
    embeddings = [[0, 1]]
    solutions = [
        [-1, +1],
        [-1, +1],
        [+1, -1],
    ]
    assert unembed_answer(solutions, embeddings, 'discard') == []


def test_vote_noties():
    embeddings = [(1, 2), [3, 5, 4]]
    solutions = [
        (3, +1, +1, -1, -1, -1),
        (3, -1, -1, -1, +1, +1),
        (3, -1, -1, +1, +1, -1),
        (3, +1, +1, -1, +1, -1)
    ]
    expected = [
        [+1, -1],
        [-1, +1],
        [-1, +1],
        [+1, -1]
    ]
    assert unembed_answer(solutions, embeddings, 'vote') == expected


def test_vote_ties():
    embeddings = [[0, 2], {1, 4}]
    solutions = [
        [+1, +1, -1, 3, -1],
        [-1, +1, +1, 3, -1],
        [+1, -1, -1, 3, +1],
        [-1, -1, +1, 3, +1]
    ]
    assert [x in (+1, -1)
            for s in unembed_answer(solutions, embeddings,  'vote')
            for x in s]


def test_weighted_random():
    # this test relies on random behaviour! yikes!
    # ensure runs and acceptance range are both suitably large
    runs = 10000
    embeddings = [(0, 3), [4, 2, 1, 5, 6, 7, 10]]
    solutions = [
        [+1, -1, -1, +1, -1, -1, -1, -1, 3, 3, -1],
        [-1, +1, +1, +1, -1, -1, -1, -1, 3, 3, -1],
    ]

    totals = [[0, 0], [0, 0]]
    for _ in xrange(runs):
        usols = unembed_answer(solutions, embeddings, 'weighted_random')
        for t, s in zip(totals, usols):
            t[0] += s[0]
            t[1] += s[1]

    assert totals[0][0] == runs
    assert totals[0][1] == -runs
    assert -400 < totals[1][0] < 400
    assert -4586 < totals[1][1] < -3986


def test_minimize_energy():
    embeddings = [(0, 5), (1, 6), (2, 7), (3, 8), (4, 10)]
    h = []
    j = {(0, 1): -1, (0, 2): 2, (0, 3): 2, (0, 4): -1,
         (2, 1): -1, (1, 3): 2, (3, 1): -1, (1, 4): -1,
         (2, 3): 1, (4, 2): -1, (2, 4): -1, (3, 4): 1}
    solutions = [
        [-1, -1, -1, -1, -1, -1, +1, +1, +1, 3, +1],
        [+1, +1, +1, +1, +1, -1, +1, -1, -1, 3, -1],
        [+1, +1, -1, +1, -1, -1, -1, -1, -1, 3, -1]
    ]
    expected = [
        [-1, -1, +1, +1, -1],
        [+1, +1, +1, -1, +1],
        [-1, -1, -1, +1, -1]
    ]
    assert (unembed_answer(solutions, embeddings, 'minimize_energy', h, j)
            == expected)


def test_minimize_energy_easy():
    embeddings = ({0, 1}, [2], (4, 5, 6))
    h = [-1]
    j = {}
    solutions = [
        [-1, -1, +1, 3, -1, -1, -1],
        [-1, +1, -1, 3, +1, +1, +1]
    ]
    expected = [
        [-1, +1, -1],
        [+1, -1, +1]
    ]
    assert (unembed_answer(solutions, embeddings, 'minimize_energy', h, j)
            == expected)
