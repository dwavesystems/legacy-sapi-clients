# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import unittest

from find_embedding_impl import find_embedding_impl

def generate_chimera_adjacency(p_rows, p_cols, cell_size):

    NUM_OF_SIDES = 2
    ret = set()

    # vertical edges
    for j in range(p_cols):
        start = cell_size * NUM_OF_SIDES * j
        for i in range(p_rows - 1):
            for k in range(cell_size):
                fm = start + k
                to = start + k + p_cols * cell_size * NUM_OF_SIDES
                ret.add((fm, to))
            start = start + p_cols * cell_size * NUM_OF_SIDES


    # horizontal edges
    for i in range(p_rows):
        start = cell_size * (NUM_OF_SIDES * p_cols * i + 1)
        for j in range(p_cols - 1):
            for k in range(cell_size):
                fm = start + k
                to = start + k + cell_size * NUM_OF_SIDES
                ret.add((fm, to))
            start += cell_size * NUM_OF_SIDES


    # inside-cell edges
    for i in range(p_rows):
        for j in range(p_cols):
            add = (i * p_cols + j) * cell_size * NUM_OF_SIDES
            for k in range(cell_size):
                for t in range(cell_size, 2 * cell_size):
                    ret.add((k + add, t + add))

    return ret

class TestFindEmbedding(unittest.TestCase):

    def setUp(self):
        pass

    def test_find_embedding(self):
        Q_size = 20
        Q = {}
        for i in range(Q_size):
            for j in range(Q_size):
                Q[(i, j)] = 1

        M = 5
        N = 5
        L = 4
        A = generate_chimera_adjacency(M, N, L)

        # fast_embedding must be a boolean
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'fast_embedding': 1})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'fast_embedding': 1.0})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'fast_embedding': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'fast_embedding': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'fast_embedding': ()})





        # max_no_improvement must be an integer >= 0
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'max_no_improvement': -1})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'max_no_improvement': -1.0})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'max_no_improvement': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'max_no_improvement': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'max_no_improvement': ()})





        # random_seed must be an integer
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'random_seed': 1.0})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'random_seed': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'random_seed': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'random_seed': ()})






        # timeout must be >= 0.0
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': -1})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': -1.0})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("NaN")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("+NaN")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("nan")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("+nan")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("-NaN")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': float("-nan")})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'timeout': ()})






        # tries must be an integer >= 0
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'tries': -1})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'tries': -1.0})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'tries': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'tries': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'tries': ()})






        # verbose parameter should be integer [0, 1]
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'verbose': -1})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'verbose': 2})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'verbose': "a"})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'verbose': []})
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'verbose': ()})




        # invalid parameter name
        self.assertRaises(RuntimeError, find_embedding_impl, Q, A, {'invalid_parameter_name': 1})

    def test_empty_Q(self):
        Q = {}

        M = 5
        N = 5
        L = 4
        A = generate_chimera_adjacency(M, N, L)

        embeddings = find_embedding_impl(Q, A, {})
        self.assertEqual(len(embeddings), 0)

    def test_empty_A(self):
        Q_size = 20
        Q = {}
        for i in range(Q_size):
            for j in range(Q_size):
                Q[(i, j)] = 1

        A = {}
        embeddings = find_embedding_impl(Q, A, {})
        self.assertEqual(len(embeddings), 0)

    def test_empty_Q_and_A(self):
        embeddings = find_embedding_impl({}, {}, {})
        self.assertEqual(len(embeddings), 0)

     #wrong embedding so no solution returned
    def test_no_embedding_found(self):
        Q = {(0, 1): 1, (0, 2): 1, (1, 2): 1};
        A = {(0, 1): 1, (1, 2): 1}
        embeddings = find_embedding_impl(Q, A, {})
        self.assertEqual(len(embeddings), 0)

def run():
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFindEmbedding)
    unittest.TextTestRunner(verbosity=2).run(suite)

if __name__== '__main__':
    unittest.main()
