# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

import sapilocal

_C4_NUMQUBITS = 128
_C4_QUBITS = range(_C4_NUMQUBITS)
_C4_COUPLERS = [[0, 4], [0, 5], [0, 6], [0, 7], [0, 32], [1, 4], [1, 5],
                [1, 6], [1, 7], [1, 33], [2, 4], [2, 5], [2, 6], [2, 7],
                [2, 34], [3, 4], [3, 5], [3, 6], [3, 7], [3, 35], [4, 12],
                [5, 13], [6, 14], [7, 15], [8, 12], [8, 13], [8, 14], [8, 15],
                [8, 40], [9, 12], [9, 13], [9, 14], [9, 15], [9, 41], [10, 12],
                [10, 13], [10, 14], [10, 15], [10, 42], [11, 12], [11, 13],
                [11, 14], [11, 15], [11, 43], [12, 20], [13, 21], [14, 22],
                [15, 23], [16, 20], [16, 21], [16, 22], [16, 23], [16, 48],
                [17, 20], [17, 21], [17, 22], [17, 23], [17, 49], [18, 20],
                [18, 21], [18, 22], [18, 23], [18, 50], [19, 20], [19, 21],
                [19, 22], [19, 23], [19, 51], [20, 28], [21, 29], [22, 30],
                [23, 31], [24, 28], [24, 29], [24, 30], [24, 31], [24, 56],
                [25, 28], [25, 29], [25, 30], [25, 31], [25, 57], [26, 28],
                [26, 29], [26, 30], [26, 31], [26, 58], [27, 28], [27, 29],
                [27, 30], [27, 31], [27, 59], [32, 36], [32, 37], [32, 38],
                [32, 39], [32, 64], [33, 36], [33, 37], [33, 38], [33, 39],
                [33, 65], [34, 36], [34, 37], [34, 38], [34, 39], [34, 66],
                [35, 36], [35, 37], [35, 38], [35, 39], [35, 67], [36, 44],
                [37, 45], [38, 46], [39, 47], [40, 44], [40, 45], [40, 46],
                [40, 47], [40, 72], [41, 44], [41, 45], [41, 46], [41, 47],
                [41, 73], [42, 44], [42, 45], [42, 46], [42, 47], [42, 74],
                [43, 44], [43, 45], [43, 46], [43, 47], [43, 75], [44, 52],
                [45, 53], [46, 54], [47, 55], [48, 52], [48, 53], [48, 54],
                [48, 55], [48, 80], [49, 52], [49, 53], [49, 54], [49, 55],
                [49, 81], [50, 52], [50, 53], [50, 54], [50, 55], [50, 82],
                [51, 52], [51, 53], [51, 54], [51, 55], [51, 83], [52, 60],
                [53, 61], [54, 62], [55, 63], [56, 60], [56, 61], [56, 62],
                [56, 63], [56, 88], [57, 60], [57, 61], [57, 62], [57, 63],
                [57, 89], [58, 60], [58, 61], [58, 62], [58, 63], [58, 90],
                [59, 60], [59, 61], [59, 62], [59, 63], [59, 91], [64, 68],
                [64, 69], [64, 70], [64, 71], [64, 96], [65, 68], [65, 69],
                [65, 70], [65, 71], [65, 97], [66, 68], [66, 69], [66, 70],
                [66, 71], [66, 98], [67, 68], [67, 69], [67, 70], [67, 71],
                [67, 99], [68, 76], [69, 77], [70, 78], [71, 79], [72, 76],
                [72, 77], [72, 78], [72, 79], [72, 104], [73, 76], [73, 77],
                [73, 78], [73, 79], [73, 105], [74, 76], [74, 77], [74, 78],
                [74, 79], [74, 106], [75, 76], [75, 77], [75, 78], [75, 79],
                [75, 107], [76, 84], [77, 85], [78, 86], [79, 87], [80, 84],
                [80, 85], [80, 86], [80, 87], [80, 112], [81, 84], [81, 85],
                [81, 86], [81, 87], [81, 113], [82, 84], [82, 85], [82, 86],
                [82, 87], [82, 114], [83, 84], [83, 85], [83, 86], [83, 87],
                [83, 115], [84, 92], [85, 93], [86, 94], [87, 95], [88, 92],
                [88, 93], [88, 94], [88, 95], [88, 120], [89, 92], [89, 93],
                [89, 94], [89, 95], [89, 121], [90, 92], [90, 93], [90, 94],
                [90, 95], [90, 122], [91, 92], [91, 93], [91, 94], [91, 95],
                [91, 123], [96, 100], [96, 101], [96, 102], [96, 103],
                [97, 100], [97, 101], [97, 102], [97, 103], [98, 100],
                [98, 101], [98, 102], [98, 103], [99, 100], [99, 101],
                [99, 102], [99, 103], [100, 108], [101, 109], [102, 110],
                [103, 111], [104, 108], [104, 109], [104, 110], [104, 111],
                [105, 108], [105, 109], [105, 110], [105, 111], [106, 108],
                [106, 109], [106, 110], [106, 111], [107, 108], [107, 109],
                [107, 110], [107, 111], [108, 116], [109, 117], [110, 118],
                [111, 119], [112, 116], [112, 117], [112, 118], [112, 119],
                [113, 116], [113, 117], [113, 118], [113, 119], [114, 116],
                [114, 117], [114, 118], [114, 119], [115, 116], [115, 117],
                [115, 118], [115, 119], [116, 124], [117, 125], [118, 126],
                [119, 127], [120, 124], [120, 125], [120, 126], [120, 127],
                [121, 124], [121, 125], [121, 126], [121, 127], [122, 124],
                [122, 125], [122, 126], [122, 127], [123, 124], [123, 125],
                [123, 126], [123, 127]]
_C4_VARORDER = [0, 32, 64, 96, 1, 33, 65, 97, 2, 34, 66, 98, 3, 35, 67, 99, 8,
                40, 72, 104, 9, 41, 73, 105, 10, 42, 74, 106, 11, 43, 75, 107,
                16, 48, 80, 112, 17, 49, 81, 113, 18, 50, 82, 114, 19, 51, 83,
                115, 24, 56, 88, 120, 25, 57, 89, 121, 26, 58, 90, 122, 27, 59,
                91, 123, 4, 5, 6, 7, 36, 37, 38, 39, 68, 69, 70, 71, 100, 101,
                102, 103, 12, 13, 14, 15, 44, 45, 46, 47, 76, 77, 78, 79, 108,
                109, 110, 111, 20, 21, 22, 23, 52, 53, 54, 55, 84, 85, 86, 87,
                116, 117, 118, 119, 28, 29, 30, 31, 60, 61, 62, 63, 92, 93, 94,
                95, 124, 125, 126, 127]


def _is_histmode(answer_mode):
    if answer_mode == 'histogram':
        return True
    elif answer_mode == 'raw':
        return False
    else:
        raise ValueError("answer_mode must be 'histogram' or 'raw'")


class _LocalSubmittedProblem(object):
    def __init__(self, solver, problem_type, problem, params):
        self._solver = solver
        self._solve_args = (problem_type, problem, params)
        self._cancelled = False

    @staticmethod
    def status():
        return {'state': 'DONE'}

    @staticmethod
    def done():
        return True

    @staticmethod
    def retry():
        pass

    def cancel(self):
        self._cancelled = True

    def result(self):
        if self._cancelled:
            raise RuntimeError('problem cancelled')
        else:
            return self._solver.solve(*self._solve_args)


class _OrangSampleSolver(object):
    _default_params = {'answer_histogram': True, 'beta': 3.0, 'num_reads': 1}
    _parameters = dict.fromkeys(('answer_mode', 'beta', 'max_answers',
                                 'num_reads', 'random_seed'), {})

    def __init__(self, num_qubits, qubits, couplers, var_order):
        self._config = {'num_vars': num_qubits,
                        'active_vars': tuple(qubits),
                        'active_var_pairs': tuple(couplers),
                        'var_order': tuple(var_order)}
        self.properties = {'supported_problem_types': ['ising', 'qubo'],
                           'parameters': self._parameters,
                           'num_qubits': num_qubits,
                           'qubits': qubits,
                           'couplers': couplers}

    def solve(self, problem_type, problem, params):
        params2 = dict(self._default_params)
        if 'answer_mode' in params:
            params2['answer_histogram'] = _is_histmode(params['answer_mode'])
        if 'beta' in params:
            params2['beta'] = params['beta']
        if 'num_reads' in params:
            params2['num_reads'] = params['num_reads']
        params2['max_answers'] = params.get('max_answers')
        if params2['max_answers'] is None:
            params2['max_answers'] = params2['num_reads']
        params2['random_seed'] = params.get('random_seed')
        params2.update(self._config)
        return sapilocal.orang_sample(problem_type, problem, params2)

    def submit(self, problem_type, problem, params):
        return _LocalSubmittedProblem(self, problem_type, problem, params)


class _OrangOptimizeSolver(object):
    _default_params = {'answer_histogram': True, 'num_reads': 1}
    _parameters = dict.fromkeys(('answer_mode', 'max_answers', 'num_reads'),
        {})

    def __init__(self, num_qubits, qubits, couplers, var_order):
        self._config = {'num_vars': num_qubits,
                        'active_vars': tuple(qubits),
                        'active_var_pairs': tuple(couplers),
                        'var_order': tuple(var_order)}
        self.properties = {'supported_problem_types': ['ising', 'qubo'],
                           'parameters': self._parameters,
                           'num_qubits': num_qubits,
                           'qubits': qubits,
                           'couplers': couplers}

    def solve(self, problem_type, problem, params):
        params2 = dict(self._default_params)
        if 'answer_mode' in params:
            params2['answer_histogram'] = _is_histmode(params['answer_mode'])
        if 'num_reads' in params:
            params2['num_reads'] = params['num_reads']
        params2['max_answers'] = params.get('max_answers')
        if params2['max_answers'] is None:
            params2['max_answers'] = params2['num_reads']
        params2.update(self._config)
        return sapilocal.orang_optimize(problem_type, problem, params2)

    def submit(self, problem_type, problem, params):
        return _LocalSubmittedProblem(self, problem_type, problem, params)


class _OrangHeuristicSolver(object):
    _default_params = {'iteration_limit': 10,
                       'max_bit_flip_prob': 1.0 / 8.0,
                       'max_local_complexity': 9,
                       'min_bit_flip_prob': 1.0 / 32.0,
                       'local_stuck_limit': 8,
                       'num_perturbed_copies': 4,
                       'num_variables': 0,
                       'time_limit_seconds': 5.0,
                       'random_seed': None}
    properties = {'supported_problem_types': ['ising', 'qubo'],
                  'parameters': _default_params}

    def solve(self, problem_type, problem, params):
        params2 = {k: params.get(k, v)
                   for k, v in self._default_params.iteritems()}
        return sapilocal.orang_heuristic(problem_type, problem, params2)

    def submit(self, problem_type, problem, params):
        return _LocalSubmittedProblem(self, problem_type, problem, params)


class _LocalConnection(object):
    def __init__(self, solvers):
        self._solvers = solvers

    def solver_names(self):
        return self._solvers.keys()

    def get_solver(self, name):
        return self._solvers[name]


local_connection = _LocalConnection({
    'c4-sw_sample': _OrangSampleSolver(_C4_NUMQUBITS,
                                       _C4_QUBITS,
                                       _C4_COUPLERS,
                                       _C4_VARORDER),
    'c4-sw_optimize': _OrangOptimizeSolver(_C4_NUMQUBITS,
                                           _C4_QUBITS,
                                           _C4_COUPLERS,
                                           _C4_VARORDER),
    'ising-heuristic': _OrangHeuristicSolver()})
