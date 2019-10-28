% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testQpAnswer %#ok<STOUT,*DEFNU>
initTestSuite
end

function testTrivialFull
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'test'), solvers));

qp = sapiremote_encodeqp(solver, []);
assertEqual(qp, struct( ...
  'format', 'qp', ...
  'lin', 'AAAAAAAA+H8AAAAAAAD4fwAAAAAAAPh/', ...
  'quad', ''))
end

function testTrivialSparse
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'test'), solvers));

qp = sapiremote_encodeqp(solver, sparse([]));
assertEqual(qp, struct( ...
  'format', 'qp', ...
  'lin', 'AAAAAAAA+H8AAAAAAAD4fwAAAAAAAPh/', ...
  'quad', ''))
end

function testEncodeFull
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'test'), solvers));

problem = [0  0 0 0   0  0;
           0  0 0 0   0  0;
           0  0 0 0   0  0;
           0  0 0 0   0  0;
           0 -1 0 0 999 -2.5];
qp = sapiremote_encodeqp(solver, problem);
assertEqual(qp, struct( ...
  'format', 'qp', ...
  'lin', 'AAAAAAAAAAAAAAAAADiPQAAAAAAAAAAA', ...
  'quad', 'AAAAAAAA8L8AAAAAAAAEwA=='))
end

function testEncodeSparse
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'test'), solvers));

problem = sparse([2 5 5], [5 5 6], [-1 999 -2.5]);
qp = sapiremote_encodeqp(solver, problem);
assertEqual(qp, struct( ...
  'format', 'qp', ...
  'lin', 'AAAAAAAAAAAAAAAAADiPQAAAAAAAAAAA', ...
  'quad', 'AAAAAAAA8L8AAAAAAAAEwA=='))
end

function testBadSolver
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'config'), solvers));

assertExceptionThrown(@() sapiremote_encodeqp(solver, []), ...
  'sapiremote:BadProblemData')
end

function testBadProblem
solvers = sapiremote_solvers(sapiremote_connection('', '', []));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'test'), solvers));

assertExceptionThrown(@() sapiremote_encodeqp(solver, [-1]), ...
  'sapiremote:BadProblemData')
assertExceptionThrown(@() sapiremote_encodeqp(solver, sparse(5, 3, 1)), ...
  'sapiremote:BadProblemData')
end
