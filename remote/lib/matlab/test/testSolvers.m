% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testSolvers %#ok<STOUT,*DEFNU>
initTestSuite
end

function testSolverNames
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
solverNames = sort(arrayfun(@(s) {s.id}, solvers(:)'));
assertEqual(solverNames, sort({'test', 'echo', 'config', 'status'}))
end

function testSolverConfig
url = 'http://example.com/sapi';
token = 'hello';
proxy = [];
conn = sapiremote_connection(url, token, proxy);
solvers = sapiremote_solvers(conn);

configSolver = solvers(arrayfun(@(s) strcmp(s.id, 'config'), solvers));
assertTrue(isscalar(configSolver))

assertEqual(configSolver.properties, ...
  struct('url', url, 'token', token, 'proxy', proxy));
end

function testBadHandle
assertExceptionThrown(@() sapiremote_solvers([]), 'sapiremote:InvalidHandle')
end
