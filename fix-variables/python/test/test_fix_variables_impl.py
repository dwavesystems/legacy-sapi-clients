# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import unittest

from fix_variables_impl import fix_variables_impl

class TestFixVariables(unittest.TestCase):

    def setUp(self):
        pass

    def test(self):
        Q = {}
        Q[(0, 0)] = 1000
        Q[(0, 1)] = -1100
        Q[(0, 2)] = 6
        Q[(1, 0)] = -1000
        Q[(1, 1)] = 600
        Q[(1, 2)] = 8000
        Q[(2, 0)] = -200
        Q[(2, 1)] = 9
        Q[(2, 2)] = -2000

        self.assertRaises(RuntimeError, fix_variables_impl, Q, "a")

        result = fix_variables_impl(Q)
        self.assertEqual(result["offset"], -2000)
        self.assertEqual(len(result["new_Q"]), 0)
        self.assertEqual(result["fixed_variables"], {0: 0, 1: 0, 2: 1})

        result = fix_variables_impl(Q, None)
        self.assertEqual(result["offset"], -2000)
        self.assertEqual(len(result["new_Q"]), 0)
        self.assertEqual(result["fixed_variables"], {0: 0, 1: 0, 2: 1})

        result = fix_variables_impl(Q, "optimized")
        self.assertEqual(result["offset"], -2000)
        self.assertEqual(len(result["new_Q"]), 0)
        self.assertEqual(result["fixed_variables"], {0: 0, 1: 0, 2: 1})

        result = fix_variables_impl(Q, "standard")
        self.assertEqual(result["offset"], -2000)
        self.assertEqual(len(result["new_Q"]), 0)
        self.assertEqual(result["fixed_variables"], {0: 0, 1: 0, 2: 1})

        # test empty Q
        result = fix_variables_impl({})
        self.assertEqual(result["offset"], 0)
        self.assertEqual(result["new_Q"], {})
        self.assertEqual(result["fixed_variables"], {})

def run():
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFixVariables)
    unittest.TextTestRunner(verbosity=2).run(suite)

if __name__== '__main__':
    unittest.main()
