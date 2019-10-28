% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testAddProblem %#ok<STOUT,*DEFNU>
initTestSuite
end

function testBadConnection
conn = 'not a connection';
assertExceptionThrown(@() sapiremote_addproblem(conn, 'x'), ...
  'sapiremote:InvalidHandle')
end

function testBadProblemId
conn = sapiremote_connection('', '');
assertExceptionThrown(@() sapiremote_addproblem(conn, 6), ...
  'sapiremote:BadArgType')
end

function testReturnType
conn = sapiremote_connection('', '');
assertTrue(isAnswerHandle(sapiremote_addproblem(conn, '123')))
end
