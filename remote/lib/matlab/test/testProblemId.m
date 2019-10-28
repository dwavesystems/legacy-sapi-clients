% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testProblemId %#ok<STOUT,*DEFNU>
initTestSuite
end

function testAvailableSubmitted
id = 'a1';
h = answerHandle('incomplete', id);
assertEqual(sapiremote_problemid(h), id)
end

function testAvailableAdded
id = 'a1';
conn = sapiremote_connection('', '', '');
h = sapiremote_addproblem(conn, id);
assertEqual(sapiremote_problemid(h), id)
end

function testUnavailableSubmitted
assertEqual(sapiremote_problemid(answerHandle('incomplete')), '')
end

function testBadHandle
assertExceptionThrown(@() sapiremote_problemid('a'), ...
  'sapiremote:InvalidHandle')
end
