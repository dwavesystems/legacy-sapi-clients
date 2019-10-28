% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testDone %#ok<STOUT,*DEFNU>
initTestSuite
end

function testIncompleteSubmitted
solvers = sapiremote_solvers(sapiremote_connection('', '', ''));
h = sapiremote_submit(solvers(1), '', '', struct);
assertFalse(sapiremote_done(h))
end

function testIncompleteAdded
conn = sapiremote_connection('', '', '');
h = sapiremote_addproblem(conn, '123');
assertFalse(sapiremote_done(h))
end

function testCompleted
h = answerHandle('answer', '', []);
assertTrue(sapiremote_done(h))
end

function testFailed
h = answerHandle('error', 'oops');
assertTrue(sapiremote_done(h))
end
