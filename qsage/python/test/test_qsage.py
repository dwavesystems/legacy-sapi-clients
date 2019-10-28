# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import unittest

from random import randint
from qsage_impl import solve_qsage_impl

def get_chimera_adjacency(M, N=None, L=None):

    ret = []

    m = M
    n = N
    k = L

    if L is None:
        k = 4
    if N is None:
        n = m

    if m <= 0:
        raise ValueError("M must be a positive integer")
    if n <= 0:
        raise ValueError("N must be a positive integer")
    if k <= 0:
        raise ValueError("K must be a positive integer")

    # vertical edges:
    for j in range(n):
        start = k * 2 * j
        for i in range(m - 1):
            for t in range(k):
                fm = start + t
                to = start + t + n * k * 2
                ret.append([fm, to])
            start = start + n * k * 2

    # horizontal edges:
    for i in range(m):
        start = k * (2 * n * i + 1)
        for j in range(n - 1):
            for t in range(k):
                fm = start + t
                to = start + t + k * 2
                ret.append([fm, to])
            start = start + k * 2

    # inside-cell edges:
    for i in range(m):
        for j in range(n):
            add = (i * n + j) * k * 2
            for t in range(k):
                for u in range(k, 2 * k):
                    ret.append([t + add, u + add])

    return ret

def calculate_energy_ising(h, J, s):

    ret = 0.0
    for k, v in J.iteritems():
        ret = ret + s[k[0]] * v * s[k[1]]

    for i in range(len(h)):
        ret = ret + h[i] * s[i]

    return ret

def mock_solve_ising(h, J):
    ret = dict()
    ret["solutions"] = []
    ret["energies"] = []
    num_solutions = 10
    for i in range(num_solutions):
        tmp = []
        for k in range(128):
            tmp.append(2 * randint(0, 1) - 1)
        ret["solutions"].append(tmp)
        ret["energies"].append(calculate_energy_ising(h, j, ret["solutions"][i]))

    #ret["num_occurrences"] = num_solutions * [1]
    return ret

class IsingSolver:
    def __init__(self):
        self.qubits = range(128)
        self.couplers = get_chimera_adjacency(4, 4, 4)
        #self.h_range = [-1, 1]
        #self.j_range = [-1, 1]

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverMissingQubits:
    def __init__(self):
        self.couplers = get_chimera_adjacency(4, 4, 4)

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverMissingCouplers:
    def __init__(self):
        self.qubits = range(128)

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverHRangeIsInvalid_1:
    def __init__(self):
        self.qubits = range(128)
        self.couplers = get_chimera_adjacency(4, 4, 4)
        self.h_range = [-1]
        self.j_range = [-1, 1]

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverHRangeIsInvalid_2:
    def __init__(self):
        self.qubits = range(128)
        self.couplers = get_chimera_adjacency(4, 4, 4)
        self.h_range = [1, -1]
        self.j_range = [-1, 1]

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverJRangeIsInvalid_1:
    def __init__(self):
        self.qubits = range(128)
        self.couplers = get_chimera_adjacency(4, 4, 4)
        self.h_range = [-1, 1]
        self.j_range = [-1]

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class IsingSolverJRangeIsInvalid_2:
    def __init__(self):
        self.qubits = range(128)
        self.couplers = get_chimera_adjacency(4, 4, 4)
        self.h_range = [-1, 1]
        self.j_range = [1, -1]

    def solve_ising(self, h, J):
        return mock_solve_ising(h, J)

class ObjClass:
    def __call__(self, states):
        num_states = len(states)
        state_len = len(states[0])
        ret = []
        for i in range(num_states):
            d1 = 0
            for j in range(state_len / 2):
                d1 = d1 + states[i][j]

            d2 = 0
            for j in range(state_len / 2, state_len):
                d2 = d2 + states[i][j]

            ret.append(d1 - d2)

        return ret

class ObjectiveClassReturnSizeWrong:
    def __call__(self, states):
        num_states = len(states)
        state_len = len(states[0])
        ret = []
        for i in range(num_states):
            d1 = 0
            for j in range(state_len / 2):
                d1 = d1 + states[i][j]

            d2 = 0
            for j in range(state_len / 2, state_len):
                d2 = d2 + states[i][j]

            ret.append(d1 - d2)

        ret.append(1) # make the return value wrong size
        return ret

class ObjectiveClassReturnTypeWrong_1:
    def __call__(self, states):
        return 1


class ObjectiveClassReturnTypeWrong_2:
    def __call__(self, states):
        num_states = len(states)
        return ['a'] * num_states


class ObjectiveClassReturnTypeWrong_3:
    def __call__(self, states):
        num_states = len(states)
        ret = [1] * (num_states - 1)
        ret.append('a')
        return ret


class ObjectiveClassNumArgumentsWrong_1:
    def __call__(self):
        pass


class ObjectiveClassNumArgumentsWrong_2:
    def __call__(self, states, dummy):
        pass


class ObjectiveClassNotCallable:
    pass

def objective_function(states):
    num_states = len(states)
    state_len = len(states[0])
    ret = []
    for i in range(num_states):
        d1 = 0
        for j in range(state_len / 2):
            d1 = d1 + states[i][j]

        d2 = 0
        for j in range(state_len / 2, state_len):
            d2 = d2 + states[i][j]

        ret.append(d1 - d2)

    return ret

def objective_function_return_size_wrong(states):
    num_states = len(states)
    state_len = len(states[0])
    ret = []
    for i in range(num_states):
        d1 = 0
        for j in range(state_len / 2):
            d1 = d1 + states[i][j]

        d2 = 0
        for j in range(state_len / 2, state_len):
            d2 = d2 + states[i][j]

        ret.append(d1 - d2)

    ret.append(1) # make the return value wrong size
    return ret

def objective_function_return_type_wrong_1(states):
    return 1


def objective_function_return_type_wrong_2(states):
    num_states = len(states)
    return ['a'] * num_states


def objective_function_return_type_wrong_3(states):
    num_states = len(states)
    ret = [1] * (num_states - 1)
    ret.append('a')
    return ret


def objective_function_num_arguments_wrong_1():
    pass


def objective_function_num_arguments_wrong_2(states, dummy):
    pass


class LPSolverClass:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub):
        f_size = len(f)
        return [1] * f_size


class LPSolverClassReturnSizeWrong:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub):
        f_size = len(f)
        return [1] * (f_size + 1)


class LPSolverClassReturnTypeWrong_1:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub):
        return 1


class LPSolverClassReturnTypeWrong_2:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub):
        f_size = len(f)
        return ['a'] * f_size


class LPSolverClassReturnTypeWrong_3:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub):
        f_size = len(f)
        ret = [1] * (f_size - 1)
        ret.append('a')
        return ret


class LPSolverClassNumArgumentsWrong_1:
    def __call__(self, f):
        pass


class LPSolverClassNumArgumentsWrong_2:
    def __call__(self, f, Aineq, bineq, Aeq, beq, lb, ub, dummy):
        pass


class LPSolverClassNotCallable:
    pass


def lp_solver(f, Aineq, bineq, Aeq, beq, lb, ub):
    f_size = len(f)
    return [1] * f_size


def lp_solver_return_size_wrong(f, Aineq, bineq, Aeq, beq, lb, ub):
    f_size = len(f)
    return [1] * (f_size + 1)


def lp_solver_return_type_wrong_1(f, Aineq, bineq, Aeq, beq, lb, ub):
    return 1


def lp_solver_return_type_wrong_2(f, Aineq, bineq, Aeq, beq, lb, ub):
    f_size = len(f)
    return ['a'] * f_size


def lp_solver_return_type_wrong_3(f, Aineq, bineq, Aeq, beq, lb, ub):
    f_size = len(f)
    ret = [1] * (f_size - 1)
    ret.append('a')
    return ret


def lp_solver_num_arguments_wrong_1(f):
    pass


def lp_solver_num_arguments_wrong_2(f, Aineq, bineq, Aeq, beq, lb, ub, dummy):
    pass


class TestQSage(unittest.TestCase):

    def setUp(self):
        pass

    def test_solve(self):
        num_vars = 12

        # no objective_function parameter provided
        self.assertRaises(TypeError, solve_qsage_impl)

        # invalid objective_function
        self.assertRaises(RuntimeError, solve_qsage_impl, 'a', num_vars, IsingSolver(), {})

        # invalid obj class (return value wrong size)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassReturnSizeWrong(), num_vars, IsingSolver(), {})

        # invalid obj class [return value is not a tuple or list]
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassReturnTypeWrong_1(), num_vars, IsingSolver(), {})

        # invalid obj class (return value is a list but contains non-numeric item)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassReturnTypeWrong_2(), num_vars, IsingSolver(), {})

        # invalid obj class (return value is a list but contains non-numeric item)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassReturnTypeWrong_3(), num_vars, IsingSolver(), {})

        # invalid obj class (number of arguments is wrong)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassNumArgumentsWrong_1(), num_vars, IsingSolver(), {})

        # invalid obj class (number of arguments is wrong)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassNumArgumentsWrong_2(), num_vars, IsingSolver(), {})

        # invalid obj class (not callable since has no __call__ method)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjectiveClassNotCallable(), num_vars, IsingSolver(), {})

        # invalid obj function (return value wrong size)
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_return_size_wrong, num_vars, IsingSolver(), {})

        # invalid obj function [return value is not a tuple or list]
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_return_type_wrong_1, num_vars, IsingSolver(), {})

        # invalid obj function (return value is a list but contains non-numeric item)
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_return_type_wrong_2, num_vars, IsingSolver(), {})

        # invalid obj function (return value is a list but contains non-numeric item)
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_return_type_wrong_3, num_vars, IsingSolver(), {})

        # invalid obj function (number of arguments is wrong)
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_num_arguments_wrong_1, num_vars, IsingSolver(), {})

        # invalid obj function (number of arguments is wrong)
        self.assertRaises(RuntimeError, solve_qsage_impl, objective_function_num_arguments_wrong_2, num_vars, IsingSolver(), {})






        # no num_vars parameter provided
        self.assertRaises(TypeError, solve_qsage_impl, None)

        # num_vars type is wrong
        self.assertRaises(TypeError, solve_qsage_impl, None, 'a', None, {})

        # invalid num_vars
        self.assertRaises(TypeError, solve_qsage_impl, ObjClass(), 'a', IsingSolver(), {})

        # invalid num_vars
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), -num_vars, IsingSolver(), {})






        # no solver parameter provided
        self.assertRaises(TypeError, solve_qsage_impl, None, None)

        # invalid solver
        self.assertRaises(RuntimeError, solve_qsage_impl, None, num_vars, None, {})

        # invalid solver
        self.assertRaises(RuntimeError, solve_qsage_impl, None, num_vars, 1, {})
        self.assertRaises(RuntimeError, solve_qsage_impl, None, num_vars, 'a', {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverMissingQubits(), {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverMissingCouplers(), {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverHRangeIsInvalid_1(), {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverHRangeIsInvalid_2(), {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverJRangeIsInvalid_1(), {})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolverJRangeIsInvalid_2(), {})







        # draw_sample must be a boolean
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'draw_sample': 1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'draw_sample': 1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'draw_sample': 'a'})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'draw_sample': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'draw_sample': ()})






        # exit_threshold_value can be any double
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': ()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("+NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("nan")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("+nan")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("-NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'exit_threshold_value': float("-nan")})






        # init_solution not a list or tuple
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': 1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': 1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': "a"})

        # init_solution contains non-integer
        initial_solution = [1] * num_vars
        initial_solution[0] = "a"
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': initial_solution})

        initial_solution = [1] * num_vars
        initial_solution[0] = 1.0
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': initial_solution})

        # init_solution's length is not the same as number of variables
        initial_solution = [1] * (num_vars - 1)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': initial_solution})

        # init_solution contains integer that is not -1/1 when ising_qubo is "ising"
        initial_solution = [1] * num_vars
        initial_solution[0] = 0
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'init_solution': initial_solution})

        # init_solution's length is not the same as number of variables when ising_qubo is "qubo"
        initial_solution = [1] * (num_vars - 1)
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': "qubo", 'init_solution': initial_solution})

        # init_solution contains integer that is not 0/1 when ising_qubo is "qubo"
        initial_solution = [1] * num_vars
        initial_solution[0] = -1
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': "qubo", 'init_solution': initial_solution})






        # ising_qubo must be a string of "ising" or "qubo"
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': 1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': 1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'ising_qubo': ()})






        # lp_solver is invalid
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': 1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassReturnSizeWrong()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassReturnTypeWrong_1()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassReturnTypeWrong_2()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassReturnTypeWrong_3()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassNumArgumentsWrong_1()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassNumArgumentsWrong_2()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': LPSolverClassNotCallable()})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_return_size_wrong})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_return_type_wrong_1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_return_type_wrong_2})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_return_type_wrong_3})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_num_arguments_wrong_1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'lp_solver': lp_solver_num_arguments_wrong_2})






        # max_num_state_evaluations must be an integer >= 0
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'max_num_state_evaluations': -1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'max_num_state_evaluations': -1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'max_num_state_evaluations': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'max_num_state_evaluations': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'max_num_state_evaluations': ()})






        # random_seed must be an integer
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'random_seed': 1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'random_seed': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'random_seed': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'random_seed': ()})






        # timeout must be >= 0.0
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': -1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': -1.0})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("+NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("nan")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("+nan")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("-NaN")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': float("-nan")})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'timeout': ()})






        # verbose parameter should be integer [0, 2]
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'verbose': -1})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'verbose': 3})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'verbose': "a"})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'verbose': []})
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'verbose': ()})





        # invalid parameter name
        self.assertRaises(RuntimeError, solve_qsage_impl, ObjClass(), num_vars, IsingSolver(), {'invalid_parameter_name': 1})


def run():
    suite = unittest.TestLoader().loadTestsFromTestCase(TestQSage)
    unittest.TextTestRunner(verbosity=2).run(suite)

if __name__== '__main__':
    unittest.main()
