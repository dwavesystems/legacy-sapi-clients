% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testFixVariablesMex %#ok<STOUT,*DEFNU>
initTestSuite
end

function testFixVariablesMexInvalidParameters

Q = ones(10);

assertExceptionThrown(@()fix_variables_mex(Q, 1), '');
assertExceptionThrown(@()fix_variables_mex(Q, 'a'), '');
assertExceptionThrown(@()fix_variables_mex(Q, ''), '');
assertExceptionThrown(@()fix_variables_mex(Q, []), '');
assertExceptionThrown(@()fix_variables_mex(Q, {}), '');

% test empty Q
[fixedVars newQ offset] = fix_variables_mex([]);
assertTrue(isempty(fixedVars));
assertEqual(full(newQ), []);
assertEqual(offset, 0);

end
