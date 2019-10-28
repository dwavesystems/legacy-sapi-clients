% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testAwaitCompletion %#ok<STOUT,*DEFNU>
initTestSuite
end

function testTimeout
h = arrayfun(@(n) {answerHandle('incomplete')}, 1:10);
t = tic;
assertEqual(sapiremote_awaitcompletion(h, 1, 0.1), false(1, 10))
t = toc(t);
assertTrue(t >= 0.1);
assertTrue(t < 1);
end

function testComplete
h = [ ...
  arrayfun(@(n) {answerHandle('incomplete')}, 1:3) ...
  arrayfun(@(n) {answerHandle('answer', '', '')}, 1:3) ...
  arrayfun(@(n) {answerHandle('error', '')}, 1:3) ];

t = tic;
assertEqual(sapiremote_awaitcompletion(h, 6, 2), [false(1,3) true(1,6)])
t = toc(t);
assertTrue(t < 0.05)
end

function testBadHandle
assertExceptionThrown(@() sapiremote_awaitcompletion({1, '4'}, 1, 1), ...
  'sapiremote:InvalidHandle')
end
