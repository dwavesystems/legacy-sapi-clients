# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import datetime
import unittest
import sapiremote


class ConnectionTest(unittest.TestCase):
    def test_solver_names(self):
        conn = sapiremote.Connection('', '')
        solver_names = set(conn.solvers().keys())
        self.assertEqual(solver_names, {'config', 'test', 'echo', 'status'})

    def test_add_problem(self):
        conn = sapiremote.Connection('', '')
        p = conn.add_problem('123')
        self.assertTrue(isinstance(p, sapiremote.SubmittedProblem))
        self.assertEqual(p.problem_id(), '123')

    def test_config_solver_with_proxy(self):
        url = 'http://example.com/sapi'
        token = 'secret'
        proxy = 'http://example.net'
        conn = sapiremote.Connection(url, token, proxy)
        solver = conn.solvers()['config']
        self.assertEqual(solver.properties(),
                         {'url': url, 'token': token, 'proxy': proxy})

    def test_config_solver_no_proxy(self):
        url = 'http://example.com/sapi'
        token = 'secret'
        proxy = None
        conn = sapiremote.Connection(url, token, proxy)
        solver = conn.solvers()['config']
        self.assertEqual(solver.properties(),
                         {'url': url, 'token': token, 'proxy': proxy})

    def test_config_solver_default_proxy(self):
        url = 'http://example.com/sapi'
        token = 'secret'
        proxy = None
        conn = sapiremote.Connection(url, token, proxy)
        solver = conn.solvers()['config']
        self.assertEqual(solver.properties(),
                         {'url': url, 'token': token, 'proxy': proxy})


class SolverTest(unittest.TestCase):
    def test_solver_submit(self):
        conn = sapiremote.Connection('', '')
        solver = conn.solvers()['test']
        self.assertTrue(isinstance(solver.submit('', None, {}),
                                   sapiremote.SubmittedProblem))


class SubmittedProblemTest(unittest.TestCase):
    def test_cancel(self):
        conn = sapiremote.Connection('', '')
        solver = conn.solvers()['test']
        problem = solver.submit('', None, {})
        self.assertFalse(problem.done())
        problem.cancel()
        self.assertTrue(problem.done())
        self.assertRaises(RuntimeError, problem.answer)

    def test_retry(self):
        solver = sapiremote.Connection('', '').solvers()['status']

        init_status = {
            'problem_id': '123',
            'state': 'FAILED',
            'last_good_state': 'SUBMITTED',
            'remote_status': 'IN_PROGRESS',
            'error_type': 'NETWORK',
            'error_message': 'nope',
            'submitted_on': '1:23',
            'solved_on': '3:45'
        }
        problem = solver.submit('', None, init_status)
        problem.retry()
        final_status = init_status.copy()
        final_status['state'] = 'RETRYING'
        self.assertEqual(problem.status(), final_status)

    def test_problem_id(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit('', None, {'problem_id': '123'})
        self.assertEqual(problem.problem_id(), '123')

    def test_not_done(self):
        self.assertFalse(sapiremote.Connection('', '').add_problem('1').done())

    def test_done_answer(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit('', None, {'answer': True})
        self.assertTrue(problem.done())

    def test_done_error(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit('', None, {'error': 'oh no'})
        self.assertTrue(problem.done())

    def test_answer(self):
        prob_type = 'magic'
        data = [None, True, False, 123, 4.5, {}]
        params = {'None': None, 'True': True, 'False': False,
                  '123': 123, '4.5': 4.5, 'array': range(20),
                  'object': {'more': {'layers': {'ok': {"that's": 'enough'}}}},
                  'answer': True}
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit(prob_type, data, params)
        self.assertEqual(problem.answer(),
            [prob_type, {'type': prob_type, 'data': data, 'params': params}])

    def test_error(self):
        prob_type = 'magic'
        data = [None, True, False, 123, 4.5, {}]
        params = {'None': None, 'True': True, 'False': False,
                  '123': 123, '4.5': 4.5, 'array': range(20),
                  'object': {'more': {'layers': {'ok': {"that's": 'enough'}}}},
                  'error': 'blah'}
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit(prob_type, data, params)
        self.assertRaises(RuntimeError, problem.answer)

    def test_unicode_keys(self):
        prob_type = 'magic'
        data = {u'a': 123}
        params = {u'p': 456, u'answer': True}
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = solver.submit(prob_type, data, params)
        sdata = {str(k): v for k, v in data.iteritems()}
        sparams = {str(k): v for k, v in params.iteritems()}
        self.assertEqual(problem.answer(),
            [prob_type, {'type': prob_type, 'data': sdata, 'params': sparams}])


class StatusTest(unittest.TestCase):
    def test_status_submitting(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'SUBMITTING',
            'last_good_state': 'SUBMITTING',
            'remote_status': 'UNKNOWN',
            'error_type': 'INTERNAL',
            'error_message': 'nope',
            'submitted_on': '',
            'solved_on': ''
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['last_good_state']
        del expected_status['error_type']
        del expected_status['error_message']
        del expected_status['submitted_on']
        del expected_status['solved_on']
        self.assertEqual(problem.status(), expected_status)

    def test_status_submitted(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'SUBMITTED',
            'last_good_state': 'SUBMITTED',
            'remote_status': 'PENDING',
            'error_type': 'INTERNAL',
            'error_message': 'nope',
            'submitted_on': 'earlier',
            'solved_on': ''
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['last_good_state']
        del expected_status['error_type']
        del expected_status['error_message']
        del expected_status['solved_on']
        self.assertEqual(problem.status(), expected_status)

    def test_status_completed(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'DONE',
            'last_good_state': 'DONE',
            'remote_status': 'COMPLETED',
            'error_type': 'INTERNAL',
            'error_message': 'nope',
            'submitted_on': '123',
            'solved_on': '234'
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['last_good_state']
        del expected_status['error_type']
        del expected_status['error_message']
        self.assertEqual(problem.status(), expected_status)

    def test_status_retrying(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'RETRYING',
            'last_good_state': 'SUBMITTED',
            'remote_status': 'PENDING',
            'error_type': 'PROTOCOL',
            'error_message': 'bad times',
            'submitted_on': 'qaz',
            'solved_on': ''
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['solved_on']
        self.assertEqual(problem.status(), expected_status)

    def test_status_failed_solve(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'DONE',
            'last_good_state': 'DONE',
            'remote_status': 'FAILED',
            'error_type': 'SOLVE',
            'error_message': 'bad times',
            'submitted_on': 'qwe',
            'solved_on': 'asd'
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['last_good_state']
        self.assertEqual(problem.status(), expected_status)

    def test_status_failed_network(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'FAILED',
            'last_good_state': 'SUBMITTING',
            'remote_status': 'UNKNOWN',
            'error_type': 'NETWORK',
            'error_message': 'bad times',
            'submitted_on': '',
            'solved_on': 'what'
        }
        problem = solver.submit('', None, expected_status)
        del expected_status['submitted_on']
        self.assertEqual(problem.status(), expected_status)

    def test_status_failed_auth(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'FAILED',
            'last_good_state': 'SUBMITTING',
            'remote_status': 'UNKNOWN',
            'error_type': 'AUTH',
            'error_message': 'bad times',
            'submitted_on': 'qwerty',
            'solved_on': 'asdf'
        }
        problem = solver.submit('', None, expected_status)
        self.assertEqual(problem.status(), expected_status)

    def test_status_failed_memory(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'FAILED',
            'last_good_state': 'SUBMITTING',
            'remote_status': 'UNKNOWN',
            'error_type': 'MEMORY',
            'error_message': 'bad times',
            'submitted_on': 'qwerty',
            'solved_on': 'asdf'
        }
        problem = solver.submit('', None, expected_status)
        self.assertEqual(problem.status(), expected_status)

    def test_status_failed_internal(self):
        solver = sapiremote.Connection('', '').solvers()['status']
        expected_status = {
            'problem_id': '123',
            'state': 'FAILED',
            'last_good_state': 'SUBMITTING',
            'remote_status': 'UNKNOWN',
            'error_type': 'INTERNAL',
            'error_message': 'bad times',
            'submitted_on': 'qwerty',
            'solved_on': 'asdf'
        }
        problem = solver.submit('', None, expected_status)
        self.assertEqual(problem.status(), expected_status)


class QpProblemTest(unittest.TestCase):
    def test_encode_trivial(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        expected = {
            'format': 'qp',
            'lin': 'AAAAAAAA+H8AAAAAAAD4fwAAAAAAAPh/',
            'quad': ''}
        self.assertEqual(sapiremote.encode_qp_problem(solver, {}),
                         expected)

    def test_encode(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        problem = {(1, 4): -1, (4, 5): -2.5, (4, 4): 999}
        expected = {
            'format': 'qp',
            'lin': 'AAAAAAAAAAAAAAAAADiPQAAAAAAAAAAA',
            'quad': 'AAAAAAAA8L8AAAAAAAAEwA=='}
        self.assertEqual(sapiremote.encode_qp_problem(solver, problem),
                         expected)

    def test_bad_solver(self):
        solver = sapiremote.Connection('', '').solvers()['config']
        self.assertRaises(RuntimeError,
                          lambda: sapiremote.encode_qp_problem(solver, {}))

    def test_bad_problem(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        def encode(p):
            def f():
                sapiremote.encode_qp_problem(solver, p)
            return f
        self.assertRaises(TypeError, encode({1: -1}))
        self.assertRaises(TypeError, encode({(1,): -1}))
        self.assertRaises(TypeError, encode({(1, 4, 5): -1}))
        self.assertRaises(TypeError, encode({(1, 4): 'three'}))
        self.assertRaises(RuntimeError, encode({(999, 888): 1}))


class QpAnswerTest(unittest.TestCase):
    def test_empty(self):
        answer = {'format': 'qp',
                  'energies': '',
                  'num_variables': 0,
                  'active_variables': '',
                  'solutions': ''}
        expected = {'energies': [], 'solutions': []}
        self.assertEqual(sapiremote.decode_qp_answer('ising', answer),
                         expected)

    def test_hist_ising(self):
        answer = {'format': 'qp',
                  'energies': 'AAAAAAAAJMAAAAAAAAAUwA==',
                  'num_variables': 5,
                  'active_variables': 'AQAAAAIAAAAEAAAA',
                  'solutions': 'AOA=',
                  'num_occurrences': '6AMAANsDAAA=',
                  'extra': 'stuff'}

        expected = {'energies': [-10.0, -5.0],
                    'solutions': [[3, -1, -1, 3, -1], [3, 1, 1, 3, 1]],
                    'num_occurrences': [1000, 987],
                    'extra': 'stuff'}
        self.assertEqual(sapiremote.decode_qp_answer('ising', answer),
                         expected)

    def test_raw_ising(self):
        answer = {'format': 'qp',
                  'energies': 'AAAAAAAAJMAAAAAAAAAUwA==',
                  'num_variables': 5,
                  'active_variables': 'AQAAAAIAAAAEAAAA',
                  'solutions': 'AOA=',
                  'extra': 'stuff'}

        expected = {'energies': [-10.0, -5.0],
                    'solutions': [[3, -1, -1, 3, -1], [3, 1, 1, 3, 1]],
                    'extra': 'stuff'}
        self.assertEqual(sapiremote.decode_qp_answer('ising', answer),
                         expected)

    def test_hist_qubo(self):
        answer = {'format': 'qp',
                  'energies': 'AAAAAAAAJMAAAAAAAAAUwA==',
                  'num_variables': 5,
                  'active_variables': 'AQAAAAIAAAAEAAAA',
                  'solutions': 'AOA=',
                  'num_occurrences': '6AMAANsDAAA=',
                  'extra': 'stuff'}

        expected = {'energies': [-10.0, -5.0],
                    'solutions': [[3, 0, 0, 3, 0], [3, 1, 1, 3, 1]],
                    'num_occurrences': [1000, 987],
                    'extra': 'stuff'}
        self.assertEqual(sapiremote.decode_qp_answer('qubo', answer),
                         expected)

    def test_raw_qubo(self):
        answer = {'format': 'qp',
                  'energies': 'AAAAAAAAJMAAAAAAAAAUwA==',
                  'num_variables': 5,
                  'active_variables': 'AQAAAAIAAAAEAAAA',
                  'solutions': 'AOA=',
                  'extra': 'stuff'}

        expected = {'energies': [-10.0, -5.0],
                    'solutions': [[3, 0, 0, 3, 0], [3, 1, 1, 3, 1]],
                    'extra': 'stuff'}
        self.assertEqual(sapiremote.decode_qp_answer('qubo', answer),
                         expected)

    def test_decode_error(self):
        self.assertRaises(RuntimeError,
                lambda: sapiremote.decode_qp_answer('qubo', {'format': 'qp'}))


class AwaitCompletionTest(unittest.TestCase):
    def test_timeout(self):
        conn = sapiremote.Connection('', '')
        prob = lambda n: conn.add_problem(str(n))
        sp = map(prob, range(50))
        t1 = datetime.datetime.now()
        self.assertFalse(sapiremote.await_completion(sp, 1, 0.1))
        t2 = datetime.datetime.now()
        self.assertTrue(t2 - t1 >= datetime.timedelta(milliseconds=100))

    def test_complete(self):
        solver = sapiremote.Connection('', '').solvers()['test']
        prob_a = lambda n: solver.submit('', None, {'answer': True})
        prob_e = lambda n: solver.submit('', None, {'error': 'x'})
        sp = map(prob_a, range(10)) + map(prob_e, range(100, 110))
        t1 = datetime.datetime.now()
        self.assertTrue(sapiremote.await_completion(sp, len(sp), 2))
        t2 = datetime.datetime.now()
        self.assertTrue(t2 - t1 < datetime.timedelta(milliseconds=200))
