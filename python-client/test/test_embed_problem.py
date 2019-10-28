# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import pytest

from dwave_sapi2.embedding import embed_problem


def test_embed_bad_chain():
    h = []
    j = {(0, 1): 1}
    embeddings = ((0, 2), (1,))
    adj = {(0, 1), (1, 2)}

    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj)


def test_embed_nonadj():
    h = []
    j = {(0, 1): 1}
    embeddings = ((0, 1), (3, 4))
    adj = {(0, 1), (1, 2), (2, 3), (3, 4)}

    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj)


def test_embed_h_too_large():
    h = [1, 2, 3]
    j = {}
    embeddings = ((0, 1), (2,))
    adj = {(0, 1), (1, 2)}

    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj)


def test_embed_j_index_too_large():
    h = []
    j = {(0, 1): -1, (0, 2): 1}
    embeddings = ((0, 1), (2,))
    adj = {(0, 1), (1, 2)}

    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj)


def test_embed_bad_hj_range():
    h = []
    j = {(0, 1): 1}
    embeddings = [[0], [1]]
    adj = {(0, 1)}
    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj, smear=True, h_range=[0, 1])
    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj, smear=True, h_range=[-1, 0])
    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj, smear=True, j_range=[0, 1])
    with pytest.raises(ValueError):
        embed_problem(h, j, embeddings, adj, smear=True, j_range=[-1, 0])


def test_embed_trivial():
    h0, j0, jc, emb = embed_problem([], {}, [], [])
    assert h0 == []
    assert j0 == {}
    assert jc == {}
    assert emb == []


def test_embed_typical():
    h = [1, 10]
    j = {(0, 1): 15, (2, 1): -8, (0, 2): 5, (2, 0): -2}
    embeddings = [[1], [2, 3], [0]]
    adj = {(0, 1), (1, 2), (2, 3), (3, 0), (2, 0)}

    expected_h0 = [0, 1, 5, 5]
    expected_j0 = {(0, 1): 3, (0, 2): -4, (0, 3): -4, (1, 2): 15}
    expected_jc = {(2, 3): -1}

    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj)
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc
    assert emb == embeddings


def test_embed_clean():
    h = [-2, 4, -5, 14]
    j = {(0, 1): 2, (1, 2): -3, (2, 3): -18, (3, 0): -7}
    embeddings = [[7, 4], [1, 9], (0, 2), {3, 5, 6, 8}]
    adj = {(0, 2), (2, 9), (9, 1), (1, 7), (7, 4), (4, 3), (3, 5), (5, 2),
           (0, 7), (4, 9), (3, 2), (5, 8), (8, 6), (6, 1), (8, 9)}

    expected_h0 = [0, 2, -5, 7, -1, 7, 0, -1, 0, 2]
    expected_j0 = {(1, 7): 1, (4, 9): 1, (2, 9): -3, (2, 3): -9, (2, 5): -9,
                   (3, 4): -7}
    expected_jc = dict.fromkeys([(4, 7), (1, 9), (3, 5)], -1)
    expected_emb = [[7, 4], [1, 9], [2], [3, 5]]
    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj, clean=True)
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc
    assert emb == expected_emb


def test_embed_clean_unused_variables():
    h = [0, 0, -1, 0, 0, 0, 1, -2]
    j = {(1, 2): 1, (1, 3): 2, (2, 3): -2, (2, 7): 1, (3, 7): -1}
    embeddings = [[10, 11, 12], [5, 6, 7], [15, 14, 13], [4, 3],
    [0, 1, 2], [16, 17, 18], [8, 9], [20, 19]]
    adj = {(0, 1), (1, 2), (2, 0), (2, 3), (3, 4), (4, 5), (5, 6), (6, 7),
    (7, 10), (10, 9), (9, 8), (10, 11), (11, 12), (12, 10), (7, 14),
    (14, 15), (14, 13), (15, 3), (14, 16), (16, 17), (17, 18), (16, 18),
    (15, 19), (19, 20), (20, 3)}

    expected_h0 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    -0.5, -0.5, 0.0, 0.0, 0.0, -1.0, -1.0]
    expected_j0 = {(4, 5): 2.0, (15, 19): 1.0, (7, 14): 1.0, (3, 20): -1.0, (3, 15): -2.0}
    expected_jc = {(5, 6): -1.0, (14, 15): -1.0, (3, 4): -1.0, (6, 7): -1.0, (19, 20): -1.0}
    expected_emb = [[10], [5, 6, 7], [15, 14], [4, 3], [0], [16], [9], [20, 19]]
    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj, clean=True)
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc
    assert emb == expected_emb


def test_embed_smear_jpos():
    h = [-6, -2, 8]
    j = {(0, 1): 3, (1, 2): 1, (0, 2): -6}
    embeddings = ({0}, [1], {2})
    adj = {(0, 1), (0, 2), (1, 2), (3, 0), (4, 3), (5, 1), (5, 6), (2, 7),
           (2, 8), (7, 8), (7, 9), (8, 9), (0, 7)}
    h_range = [-4, 1]
    j_range = [-6, 3]

    expected_h0 = [-3, -2, 2, -3, 0, 0, 0, 2, 2, 2]
    expected_j0 = {(0, 1): 3, (1, 2): 1, (0, 2): -3, (0, 7): -3}
    expected_jc = dict.fromkeys([(0, 3), (2, 7), (2, 8), (7, 8), (7, 9),
                                 (8, 9)], -1)
    expected_emb = [{0, 3}, {1}, {2, 7, 8, 9}]
    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj, smear=True,
                                    h_range=h_range, j_range=j_range)
    assert [set(e) for e in emb] == expected_emb
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc


def test_embed_smear_jneg():
    h = [80, 100, -12]
    j = {(0, 1): -60, (1, 2): 8, (3, 2): 1}
    embeddings = ({0}, [1], {2}, (3,))
    adj = {(0, 1), (1, 2), (2, 3)} | set((x, y) for x in range(4)
                                         for y in range(4, 7))
    h_range = [-1, 1]
    j_range = [-10, 12]

    expected_h0 = [80, 25, -12, 0, 25, 25, 25]
    expected_j0 = {(0, 1): -15, (0, 4): -15, (0, 5): -15, (0, 6): -15,
                   (1, 2): 2, (2, 4): 2, (2, 5): 2, (2, 6): 2, (2, 3): 1}
    expected_jc = dict.fromkeys([(1, 4), (1, 5), (1, 6)], -1)
    expected_emb = [{0}, {1, 4, 5, 6}, {2}, {3}]
    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj, smear=True,
                                    h_range=h_range, j_range=j_range)
    assert [set(e) for e in emb] == expected_emb
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc


def test_embed_smear_empty_j():
    h = [1, 2, 3]
    j = {}
    embeddings = [[0], [1], [2]]
    adj = [(0, 1), (0, 2), (2, 1)]
    h_range = [-1, 1]
    j_range = [-30, 30]

    expected_h0 = [1, 2, 3]
    expected_j0 = {}
    expected_jc = {}
    expected_emb = [{0}, {1}, {2}]
    h0, j0, jc, emb = embed_problem(h, j, embeddings, adj, smear=True,
                                    h_range=h_range, j_range=j_range)
    assert [set(e) for e in emb] == expected_emb
    assert h0 == expected_h0
    assert j0 == expected_j0
    assert jc == expected_jc
