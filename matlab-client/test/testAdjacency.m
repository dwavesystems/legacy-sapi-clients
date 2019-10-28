% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testAdjacency %#ok<STOUT,*DEFNU>
initTestSuite
end



function testGetChimeraAdjacency

% m is non-positive
assertExceptionThrown(@()getChimeraAdjacency(0, 2, 2), '');
assertExceptionThrown(@()getChimeraAdjacency(-1, 2, 2), '');

% n is non-positive
assertExceptionThrown(@()getChimeraAdjacency(2, 0, 2), '');
assertExceptionThrown(@()getChimeraAdjacency(2, -1, 2), '');

% k is non-positive
assertExceptionThrown(@()getChimeraAdjacency(2, 2, 0), '');
assertExceptionThrown(@()getChimeraAdjacency(2, 2, -1), '');

expectedA = sparse( ...
  [1 2 1 2 3 5 6 4 5 6 7 9 10 8 9 10 1 2 13 14 13 14 5 6 15 17 18 16 17 18 9 10 19 21 22 20 21 22], ...
  [3 3 4 4 7 7 7 8 8 8 11 11 11 12 12 12 13 14 15 15 16 16 17 18 19 19 19 20 20 20 21 22 23 23 23 24 24 24], ...
  1, 24, 24);
expectedA = expectedA + expectedA';

assertEqual(getChimeraAdjacency(2, 3, 2), expectedA);

% default k
assertEqual(getChimeraAdjacency(6, 5), getChimeraAdjacency(6, 5 ,4));

% default n, k
assertEqual(getChimeraAdjacency(8), getChimeraAdjacency(8, 8, 4));

end



function testGetHardwareAdjacency

solver = struct('property', struct( ...
  'num_qubits', 8, ...
  'qubits', [0, 1, 4, 5, 7], ...
  'couplers', [0 1; 1 4; 1 5; 4 5; 5 7; 0 7]'));
adj = getHardwareAdjacency(solver);

expectedAdj = sparse([1 1 2 2 5 6], [2 8 5 6 6 8], 1, 8, 8);
expectedAdj = expectedAdj + expectedAdj';
assertEqual(adj, expectedAdj)

end
