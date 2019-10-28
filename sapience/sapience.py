# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from argparse import ArgumentParser
from array import array
from math import isnan
from base64 import b64decode, b64encode
from struct import pack
import sys
import json

import sapilocal

C4_NUMQUBITS = 128
C4_COUPLERS = (
    (0, 4), (0, 5), (0, 6), (0, 7), (0, 32), (1, 4), (1, 5), (1, 6), (1, 7),
    (1, 33), (2, 4), (2, 5), (2, 6), (2, 7), (2, 34), (3, 4), (3, 5), (3, 6),
    (3, 7), (3, 35), (4, 12), (5, 13), (6, 14), (7, 15), (8, 12), (8, 13),
    (8, 14), (8, 15), (8, 40), (9, 12), (9, 13), (9, 14), (9, 15), (9, 41),
    (10, 12), (10, 13), (10, 14), (10, 15), (10, 42), (11, 12), (11, 13),
    (11, 14), (11, 15), (11, 43), (12, 20), (13, 21), (14, 22), (15, 23),
    (16, 20), (16, 21), (16, 22), (16, 23), (16, 48), (17, 20), (17, 21),
    (17, 22), (17, 23), (17, 49), (18, 20), (18, 21), (18, 22), (18, 23),
    (18, 50), (19, 20), (19, 21), (19, 22), (19, 23), (19, 51), (20, 28),
    (21, 29), (22, 30), (23, 31), (24, 28), (24, 29), (24, 30), (24, 31),
    (24, 56), (25, 28), (25, 29), (25, 30), (25, 31), (25, 57), (26, 28),
    (26, 29), (26, 30), (26, 31), (26, 58), (27, 28), (27, 29), (27, 30),
    (27, 31), (27, 59), (32, 36), (32, 37), (32, 38), (32, 39), (32, 64),
    (33, 36), (33, 37), (33, 38), (33, 39), (33, 65), (34, 36), (34, 37),
    (34, 38), (34, 39), (34, 66), (35, 36), (35, 37), (35, 38), (35, 39),
    (35, 67), (36, 44), (37, 45), (38, 46), (39, 47), (40, 44), (40, 45),
    (40, 46), (40, 47), (40, 72), (41, 44), (41, 45), (41, 46), (41, 47),
    (41, 73), (42, 44), (42, 45), (42, 46), (42, 47), (42, 74), (43, 44),
    (43, 45), (43, 46), (43, 47), (43, 75), (44, 52), (45, 53), (46, 54),
    (47, 55), (48, 52), (48, 53), (48, 54), (48, 55), (48, 80), (49, 52),
    (49, 53), (49, 54), (49, 55), (49, 81), (50, 52), (50, 53), (50, 54),
    (50, 55), (50, 82), (51, 52), (51, 53), (51, 54), (51, 55), (51, 83),
    (52, 60), (53, 61), (54, 62), (55, 63), (56, 60), (56, 61), (56, 62),
    (56, 63), (56, 88), (57, 60), (57, 61), (57, 62), (57, 63), (57, 89),
    (58, 60), (58, 61), (58, 62), (58, 63), (58, 90), (59, 60), (59, 61),
    (59, 62), (59, 63), (59, 91), (64, 68), (64, 69), (64, 70), (64, 71),
    (64, 96), (65, 68), (65, 69), (65, 70), (65, 71), (65, 97), (66, 68),
    (66, 69), (66, 70), (66, 71), (66, 98), (67, 68), (67, 69), (67, 70),
    (67, 71), (67, 99), (68, 76), (69, 77), (70, 78), (71, 79), (72, 76),
    (72, 77), (72, 78), (72, 79), (72, 104), (73, 76), (73, 77), (73, 78),
    (73, 79), (73, 105), (74, 76), (74, 77), (74, 78), (74, 79), (74, 106),
    (75, 76), (75, 77), (75, 78), (75, 79), (75, 107), (76, 84), (77, 85),
    (78, 86), (79, 87), (80, 84), (80, 85), (80, 86), (80, 87), (80, 112),
    (81, 84), (81, 85), (81, 86), (81, 87), (81, 113), (82, 84), (82, 85),
    (82, 86), (82, 87), (82, 114), (83, 84), (83, 85), (83, 86), (83, 87),
    (83, 115), (84, 92), (85, 93), (86, 94), (87, 95), (88, 92), (88, 93),
    (88, 94), (88, 95), (88, 120), (89, 92), (89, 93), (89, 94), (89, 95),
    (89, 121), (90, 92), (90, 93), (90, 94), (90, 95), (90, 122), (91, 92),
    (91, 93), (91, 94), (91, 95), (91, 123), (96, 100), (96, 101), (96, 102),
    (96, 103), (97, 100), (97, 101), (97, 102), (97, 103), (98, 100),
    (98, 101), (98, 102), (98, 103), (99, 100), (99, 101), (99, 102),
    (99, 103), (100, 108), (101, 109), (102, 110), (103, 111), (104, 108),
    (104, 109), (104, 110), (104, 111), (105, 108), (105, 109), (105, 110),
    (105, 111), (106, 108), (106, 109), (106, 110), (106, 111), (107, 108),
    (107, 109), (107, 110), (107, 111), (108, 116), (109, 117), (110, 118),
    (111, 119), (112, 116), (112, 117), (112, 118), (112, 119), (113, 116),
    (113, 117), (113, 118), (113, 119), (114, 116), (114, 117), (114, 118),
    (114, 119), (115, 116), (115, 117), (115, 118), (115, 119), (116, 124),
    (117, 125), (118, 126), (119, 127), (120, 124), (120, 125), (120, 126),
    (120, 127), (121, 124), (121, 125), (121, 126), (121, 127), (122, 124),
    (122, 125), (122, 126), (122, 127), (123, 124), (123, 125), (123, 126),
    (123, 127))
C4_VARORDER = (
    0, 32, 64, 96, 1, 33, 65, 97, 2, 34, 66, 98, 3, 35, 67, 99, 8, 40, 72, 104,
    9, 41, 73, 105, 10, 42, 74, 106, 11, 43, 75, 107, 16, 48, 80, 112, 17, 49,
    81, 113, 18, 50, 82, 114, 19, 51, 83, 115, 24, 56, 88, 120, 25, 57, 89,
    121, 26, 58, 90, 122, 27, 59, 91, 123, 4, 5, 6, 7, 36, 37, 38, 39, 68, 69,
    70, 71, 100, 101, 102, 103, 12, 13, 14, 15, 44, 45, 46, 47, 76, 77, 78, 79,
    108, 109, 110, 111, 20, 21, 22, 23, 52, 53, 54, 55, 84, 85, 86, 87, 116,
    117, 118, 119, 28, 29, 30, 31, 60, 61, 62, 63, 92, 93, 94, 95, 124, 125,
    126, 127)

IGNORE_PARAMS = ['priority', 'problem_id', 'auto_scale', 'annealing_time',
                 'programming_thermalization', 'readout_thermalization',
                 'postprocess', 'chains', 'num_spin_reversal_transforms']
REAL_OPT_PARAMS = {'answer_mode', 'num_reads', 'max_answers'}
VALID_OPT_PARAMS = REAL_OPT_PARAMS.union(IGNORE_PARAMS)
REAL_SAMP_PARAMS = {'answer_mode', 'num_reads', 'max_answers', 'beta'}
VALID_SAMP_PARAMS = REAL_SAMP_PARAMS.union(IGNORE_PARAMS)

MAX_NUM_READS = 1000

OPT_INFO = {
    'supported_problem_types': ['ising', 'qubo'],
    'num_qubits': C4_NUMQUBITS,
    'working_qubits': list(range(C4_NUMQUBITS)),
    'working_couplers': C4_COUPLERS,
    'parameters': dict.fromkeys(REAL_OPT_PARAMS),
    'num_reads_range': [0, MAX_NUM_READS],
    'topology' : {"type": "chimera", "shape": [4, 4, 4]},
    'tags' : []
}

SAMP_INFO = OPT_INFO.copy()
SAMP_INFO.update({
    'parameters': dict.fromkeys(REAL_SAMP_PARAMS),
    'default_beta': 3.0,
    'beta_range': [0.0, sys.float_info.max]
})


class UserError(Exception):
    def __init__(self, msg):
        self.message = msg
        super(UserError, self).__init__(msg)


class ByteToStringEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, bytes):
            return obj.decode('utf-8')
        else:
            return super(ByteToStringEncoder, self).default(obj)


def decode_c4_problem(lin_bytes, quad_bytes):
    if len(lin_bytes) != C4_NUMQUBITS * 8:
        raise ValueError('lin bytes have incorrect length')
    lin = array('d', lin_bytes)
    problem = {(i, i): v for i, v in enumerate(lin) if not isnan(v)}
    active_qubits = {i[0] for i in problem}
    active_couplers = [(i, j) for i, j in C4_COUPLERS if
                       i in active_qubits and j in active_qubits]
    if len(quad_bytes) != len(active_couplers) * 8:
        raise ValueError('quad bytes have incorrect length')
    quad = array('d', quad_bytes)
    problem.update({k: v for k, v in zip(active_couplers, quad)})
    return problem, active_qubits, active_couplers


def _check_param_names(params, valid_names):
    param_names = {str(k) if isinstance(k, str) else k for k in params}
    bad = param_names.difference(valid_names)
    if bad:
        raise UserError("invalid parameter {p}".format(p=bad.pop()))


def orang_config(active_qubits, active_couplers):
    varorder = [i for i in C4_VARORDER if i in active_qubits]
    return {'num_vars': C4_NUMQUBITS, 'active_vars': varorder,
            'var_order': varorder, 'active_var_pairs': tuple(active_couplers)}


def _param_answer_histogram(user_params):
    answer_mode = user_params.get('answer_mode', 'histogram')
    if answer_mode == 'histogram':
        return True
    elif answer_mode == 'raw':
        return False
    else:
        raise UserError("answer_mode must be 'histogram' or 'raw'")


def _param_num_reads(user_params):
    if 'num_reads' in user_params:
        try:
            r = int(user_params['num_reads'])
            if not 0 <= r <= MAX_NUM_READS:
                raise UserError('num_reads out of range')
            return r
        except ValueError:
            raise UserError('invalid num_reads value')
    else:
        return 1


def _param_max_answers(user_params, params):
    if 'max_answers' in user_params:
        try:
            r = int(user_params['max_answers'])
            if r < 0:
                raise UserError('max_answers must be non-negative')
            return r
        except ValueError:
            raise UserError('invalid max_answers value')
    else:
        return params['num_reads']


def _param_beta(user_params):
    if 'beta' in user_params:
        try:
            r = float(user_params['beta'])
            if not 0 <= r < float('inf'):
                raise UserError('beta must be finite and non-negative')
            return r
        except ValueError:
            raise UserError('invalid beta value')
    else:
        return 3.0


def optimization_params(user_params, active_qubits, active_couplers):
    _check_param_names(user_params, VALID_OPT_PARAMS)
    params = {'answer_histogram': _param_answer_histogram(user_params),
              'num_reads': _param_num_reads(user_params)}
    params['max_answers'] = _param_max_answers(user_params, params)
    params.update(orang_config(active_qubits, active_couplers))
    return params


def sampling_params(user_params, active_qubits, active_couplers):
    _check_param_names(user_params, VALID_SAMP_PARAMS)
    params = {'answer_histogram': _param_answer_histogram(user_params),
              'num_reads': _param_num_reads(user_params),
              'beta': _param_beta(user_params)}
    params['max_answers'] = _param_max_answers(user_params, params)
    params.update(orang_config(active_qubits, active_couplers))
    return params


def _encode_vals(typechar, vals):
    return b64encode(pack('<{n}{t}'.format(n=len(vals), t=typechar), *vals))


def encode_sols(sols):
    encoded = array('B')
    for sol in sols:
        byte = 0
        bit = 128
        for s in sol:
            if s == 1:
                byte |= bit
            if s != 3:
                bit >>= 1
                if bit == 0:
                    encoded.append(byte)
                    byte = 0
                    bit = 128
        if bit != 128:
            encoded.append(byte)
    return b64encode(encoded.tobytes())


def encode_answer(answer):
    sols = answer['solutions']
    if sols:
        active_variables = [i for i, v in enumerate(sols[0]) if v != 3]
    else:
        active_variables = []

    answer2 = {
        'format': 'qp',
        'num_variables': C4_NUMQUBITS,
        'active_variables': _encode_vals('i', active_variables),
        'energies': _encode_vals('d', answer['energies']),
        'solutions': encode_sols(answer['solutions']),
        'timing': {}
    }
    if 'num_occurrences' in answer:
        answer2['num_occurrences'] = _encode_vals('i',
                                                  answer['num_occurrences'])
    return answer2


def parse_problem_data(data):
    try:
        if set(data) != {'format', 'lin', 'quad'} or data['format'] != 'qp':
            raise ValueError()
        lin = b64decode(data['lin'])
        quad = b64decode(data['quad'])
        return decode_c4_problem(lin, quad)
    except Exception:
        raise UserError('unable to parse problem data')


SOLVERS = {
    'c4-optimize': (sapilocal.orang_optimize, optimization_params, OPT_INFO),
    'c4-sample': (sapilocal.orang_sample, sampling_params, SAMP_INFO)
}


def solve(solver, data):
    solvefn, paramfn = SOLVERS[solver][:2]
    problem, activeq, activec = parse_problem_data(data['data'])
    params = paramfn(data['params'], activeq, activec)
    answer = solvefn(data['type'].encode("utf-8"), problem, params)
    return encode_answer(answer)


def parse_args(args):
    parser = ArgumentParser()
    parser.add_argument('action', choices=['solve', 'info'])
    parser.add_argument('solver', choices=list(SOLVERS.keys()))
    return parser.parse_args(args)


def write_json(data, out):
    json.dump(data, out, separators=(',', ':'), allow_nan=False, cls=ByteToStringEncoder)
    out.write('\n')


def run(cmdargs, stdin, stdout):
    args = parse_args(cmdargs)
    if args.action == 'solve':
        try:
            write_json(solve(args.solver, json.load(stdin)), stdout)
            return 0
        except UserError as e:
            write_json({'error_type': 'user', 'error_message': e.message},
                       stdout)
            return 1
        except Exception as e:
            write_json({'error_type': 'internal', 'error_message': e.message},
                       stdout)
            return 1
    else:
        write_json(SOLVERS[args.solver][2], stdout)


def main():
    return run(sys.argv[1:], sys.stdin, sys.stdout)


if __name__ == '__main__':
    sys.exit(main())
