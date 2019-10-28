# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

import sapiremote
import datetime


class _RemoteSubmittedProblem(object):
    def __init__(self, submitted_problem):
        self.submitted_problem = submitted_problem

    def status(self):
        rstat = self.submitted_problem.status()
        if 'submitted_on' in rstat:
            rstat['time_received'] = rstat.pop('submitted_on')
        if 'solved_on' in rstat:
            rstat['time_solved'] = rstat.pop('solved_on')
        return rstat

    def done(self):
        return self.submitted_problem.done()

    def result(self):
        answer = self.submitted_problem.answer()
        if answer[0] in ('ising', 'qubo'):
            answer = sapiremote.decode_qp_answer(answer[0], answer[1])
        return answer

    def retry(self):
        self.submitted_problem.retry()

    def cancel(self):
        self.submitted_problem.cancel()


class _RemoteSolver(object):
    def __init__(self, solver):
        self._solver = solver
        self.properties = self._solver.properties()

    def solve(self, problem_type, problem, params):
        if problem_type in ('ising', 'qubo'):
            problem = sapiremote.encode_qp_problem(self._solver, problem)
        answer = self._solver.solve(problem_type, problem, params)
        if problem_type in ('ising', 'qubo'):
            answer = sapiremote.decode_qp_answer(answer[0], answer[1])
        return answer

    def submit(self, problem_type, problem, params):
        if problem_type in ('ising', 'qubo'):
            problem = sapiremote.encode_qp_problem(self._solver, problem)
        return _RemoteSubmittedProblem(self._solver.submit(problem_type, problem, params))


class RemoteConnection(object):
    """
    RemoteConnection constructor

    remote_connection = RemoteConnection(url, token, proxy=None)

    Args:
       url: A string for url

       token: A string for token

       proxy: A string for proxy url
              Cause requests to go through a proxy. If proxy is given, it must be
              a string url of proxy. The default is to read the list of proxies
              from the environment variables <protocol>_proxy. If no proxy environment
              variables are set, in a Windows environment, proxy settings are obtained
              from the registry's Internet Settings section and in a Mac OS X environment,
              proxy information is retrieved from the OS X System Configuration Framework.

              To disable autodetected proxy pass an empty string.

    Returns:
       A RemoteConnection Python object

    Raises:
       TypeError: type mismatch
       RuntimeError: error occurred at run time
    """
    def __init__(self, url, token, proxy=None):
        #make sure the url and token input is a string type
        url = str(url)
        token = str(token)
        if proxy is not None:
            proxy = str(proxy)
        self._connection = sapiremote.Connection(url, token, proxy)
        self.solvers = dict((k, _RemoteSolver(v))
                            for k, v in self._connection.solvers().iteritems())

    """
    Get all solvers' names

    solver_names = Connection.solver_names()

    Returns:
       A tuple which includes all solvers' names
    """
    def solver_names(self):
        return self.solvers.keys()

    """
    Get a solver object whose name is solver_name

    solver = Connection.get_solver(name)

    Args:
       name: A string which is a solver's name

    Returns:
       A solver object

    Raises:
       TypeError: type mismatch
       ValueError: bad parameter value
       RuntimeError: error occurred at run time
    """
    def get_solver(self, name):
        return self.solvers[name]


def _await_remote_completion(submitted_problems, min_done, endtime):
    remote_problems = [sp.submitted_problem for sp in submitted_problems]
    done = False
    while not done and datetime.datetime.now() < endtime:
        t = min((endtime - datetime.datetime.now()).total_seconds(), 1)
        done = sapiremote.await_completion(remote_problems, min_done, t)

    return done
