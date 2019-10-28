# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import dwave_sapi2.core
import dwave_sapi2.local


class TestSolver(object):
    def __init__(self):
        self.properties = {}

    @staticmethod
    def solve(problem_type, problem, params):
        return {'problem_type': problem_type, 'problem': problem,
                'params': params}

    def submit(self, problem_type, problem, params):
        return dwave_sapi2.local._LocalSubmittedProblem(self, problem_type,
                                                        problem, params)


def test_solve(monkeypatch):
    def dummy_await_completion(_, __, timeout):
        return True

    monkeypatch.setattr('sapiremote.await_completion', dummy_await_completion)

    probtype = 'qwerty'
    problem = range(100)
    params = {'param_a': 4}
    result = dwave_sapi2.core._solve(TestSolver(), probtype, problem, params)
    assert result == {'problem_type': probtype, 'problem': problem,
                      'params': params}
