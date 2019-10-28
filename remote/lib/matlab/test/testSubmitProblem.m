% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testSubmitProblem %#ok<STOUT,*DEFNU>
initTestSuite
end

function testSuccess
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
assertTrue(isAnswerHandle( ...
  sapiremote_submit(solvers(1), 'a', 1:5, struct('b', 123))))
assertTrue(isAnswerHandle( ...
  sapiremote_submit(solvers(1), '', [], struct())))
end

function testUnknownSolver
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
badSolver = solvers(1);
badSolver.id = 'bogus';

assertExceptionThrown(@() ...
  sapiremote_submit(badSolver, '', [], struct()), 'sapiremote:InvalidHandle')
end

function testBadProblemType
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
assertExceptionThrown(@() ...
  sapiremote_submit(solvers(1), {}, [], struct()), 'sapiremote:BadArgType')
end

function testBadProblemData
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
assertExceptionThrown(@() ...
  sapiremote_submit(solvers(1), '', @(x) x+1, struct()), ...
  'sapiremote:BadProblemData')
end

function testBadParams
conn = sapiremote_connection('', '');
solvers = sapiremote_solvers(conn);
assertExceptionThrown(@() ...
  sapiremote_submit(solvers(1), '', [], 'none'), 'sapiremote:BadProblemData')
end
