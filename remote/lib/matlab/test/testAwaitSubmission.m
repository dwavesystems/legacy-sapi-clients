% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testAwaitSubmission %#ok<STOUT,*DEFNU>
initTestSuite
end

function testTimeout
h = arrayfun(@(n) {answerHandle('incomplete')}, 1:10);
t = tic;
assertEqual(sapiremote_awaitsubmission(h, 0.1), repmat({[]}, 1, 10))
t = toc(t);
assertTrue(t >= 0.1);
assertTrue(t < 1);
end

function testComplete
h = [ ...
  arrayfun(@(n) {answerHandle('incomplete', num2str(n))}, 1:3) ...
  arrayfun(@(n) {answerHandle('answer', '', '', num2str(n))}, 11:13) ...
  arrayfun(@(n) {answerHandle('error', '', num2str(n))}, 21:23) ];

t = tic;
nhs = sapiremote_awaitsubmission(h, 2);
t = toc(t);
assertTrue(t < 0.05)
for nh=nhs
  assertTrue(isAnswerHandle(nh{1}))
  assertTrue(isfield(nh{1}, 'problem_id'))
end
end

function testBadHandle
assertExceptionThrown(@() sapiremote_awaitsubmission({1, '4'}, 1), ...
  'sapiremote:InvalidHandle')
end
