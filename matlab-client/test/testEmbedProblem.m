% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testEmbedProblem %#ok<STOUT,*DEFNU>
initTestSuite
end



function testEmbedBadChain
h = [];
J = [0 1; 0 0];
embeddings = {[0 2], 1};
A = sparse(1:2, 2:3, 1);

assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A), ...
                      'sapi:BadArg')
end



function testEmbedNonadj
h = [];
J = [0 1; 0 0];
embeddings = {[0 1], [3 4]};
A = sparse(1:4, 2:5, 1);

assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A), ...
                      'sapi:BadArg')
end



function testEmbedHTooLarge
h = 1:3;
J = [];
embeddings = {[0 1], 2};
A = sparse(1:2, 2:3, 1);

assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A), ...
                      'sapi:BadArg')
end



function testEmbedJIndexTooLarge
h = [];
J = [0 1 2];
embeddings = {[0 1], 2};
A = sparse(1:2, 2:3, 1);

assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A), ...
                      'sapi:BadArg')
end



function testEmbedBadHJRange
h = [];
J = [0 1];
embeddings = {0, 1};
A = [0 1];

assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A, false, ...
    true, [ 0 1], [-1 1]), 'sapi:BadArg')
assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A, false, ...
    true, [-1 0], [-1 1]), 'sapi:BadArg')
assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A, false, ...
    true, [-1 1], [ 0 1]), 'sapi:BadArg')
assertExceptionThrown(@() sapiEmbedProblem(h, J, embeddings, A, false, ...
    true, [-1 1], [-1 0]), 'sapi:BadArg')
end



function testEmbedTrivial
[h0, J0, Jc, emb] = sapiEmbedProblem([], [], {}, []);

assertTrue(isempty(h0))
assertTrue(isempty(J0))
assertTrue(isempty(Jc))
assertTrue(isempty(emb))
end



function testEmbedTypical
h = [1 10];
J = sparse([1 3 1 3], [2 2 3 1], [15 -8 5 -2]);
embeddings = {1, [2 3], 0};
A = sparse([1 2 3 4 3], [2 3 4 1 1], 1);

expectedh0 = [0 1 5 5]';
expectedJ0 = sparse([1 1 1 2], [2 3 4 3], [3 -4 -4 15], 4, 4);
expectedJc = sparse(3, 4, -1, 4, 4);

[h0, J0, Jc, emb] = sapiEmbedProblem(h, J, embeddings, A);
assertEqual(h0, expectedh0)
assertEqual(J0, expectedJ0)
assertEqual(Jc, expectedJc)
assertEqual(emb, embeddings)
end



function testEmbedClean
h = [-2 4 -5 14];
J = sparse([1 2 3 4], [2 3 4 1], [2 -3 -18 -7]);
embeddings = {[7 4], [1 9], [0 2], [3 5 6 8]};
A = sparse([1 3 10 2 8 5 4 6 1 5 4 6 9 7 9], ...
           [3 10 2 8 5 4 6 3 8 10 3 9 7 2 10], 1);

expectedh0 = [0 2 -5 7 -1 7 0 -1 0 2]';
expectedJ0 = sparse([2 5 3 3 3 4], [8 10 10 4 6 5], [1 1 -3 -9 -9 -7], 10, 10);
expectedJc = sparse([5 2 4], [8 10 6], -1, 10, 10);
expectedEmb = {[7 4], [1 9], 2, [3 5]};
[h0, J0, Jc, emb] = sapiEmbedProblem(h, J, embeddings, A, true);
assertEqual(h0, expectedh0)
assertEqual(J0, expectedJ0)
assertEqual(Jc, expectedJc)
assertEqual(emb, expectedEmb)
end


function testEmbedCleanUnUsedVariables
h = [0 0 -1 0 0 0 1 -2];
J = sparse([2 2 3 3 4], [3 4 4 8 8], [1 2 -2 1 -1], 8, 8);
embeddings = {[10 11 12], [5 6 7], [15 14 13], [4 3], [0 1 2], ...
    [16 17 18], [8 9], [20 19]};
A = sparse([1 2 3 3 4 5 6 7 8 11 10 11 12 13 8 15 15 16 15 17 18 17 16 20 21], ...
    [2 3 1 4 5 6 7 8 11 10 9 12 13 11 15 16 14 4 17 18 19 19 20 21 4], 1);

expectedh0 = [0 0 0 0 0 0 0 0 1.0 0 0 0 0 0 -0.5 -0.5 0 0 0 -1.0 -1.0]';
expectedJ0 = sparse([5 8 4 16 4], [6 15 16 20 21], [2 1 -2 1 -1], 21, 21);
expectedJc = sparse([4 6 7 15 20], [5 7 8 16 21], [-1 -1 -1 -1 -1] 21, 21);
expectedEmb = {[10], [5, 6, 7], [15, 14], [4, 3], [0], [16], [9], [20, 19]};
[h0, J0, Jc, newEmb] = sapiEmbedProblem(h, J, embeddings, A, true);
assertEqual(h0, expectedh0)
assertEqual(J0, expectedJ0)
assertEqual(Jc, expectedJc)
assertEqual(newEmb, expectedEmb)
end


function testEmbedSmearJpos
h = [-6 -2 8];
J = sparse([1 2 1], [2 3 3], [3 1 -6]);
embeddings = {0, 1, 2};
A = sparse([1 1 2 4 5 6 6 3 3 8 8 9 1], [2 3 3 1 4 2 7 8 9 9 10 10 8], 1);
hRange = [-4 1];
JRange = [-6 3];

expectedh0 = [-3 -2 2 -3 0 0 0 2 2 2]';
expectedJ0 = sparse([1 2 1 1], [2 3 3 8], [3 1 -3 -3], 10, 10);
expectedJc = sparse([1 3 3 8 8 9], [4 8 9 9 10 10], -1, 10, 10);
expectedEmb = {[0 3], 1, [2, 7, 8, 9]};
[h0, J0, Jc, emb] = sapiEmbedProblem(h, J, embeddings, A, false, ...
                                     true, hRange, JRange);
assertEqual(h0, expectedh0)
assertEqual(J0, expectedJ0)
assertEqual(Jc, expectedJc)
emb = cellfun(@sort, emb, 'UniformOutput', false);
assertEqual(emb, expectedEmb)
end


function testEmbedSmearJneg
h = [80 100 -12];
J = sparse([1 2 4], [2 3 3], [-60 8 1]);
embeddings = {0, 1, 2, 3};
A = sparse(1:3, 2:4, 1, 7, 7) + [zeros(4) ones(4, 3); zeros(3, 7)];
hRange = [-1 1];
JRange = [-10 12];

expectedh0 = [80 25 -12 0 25 25 25]';
expectedJ0 = sparse([1 1 1 1 2 3 3 3 3], [2 5 6 7 3 5 6 7 4], ...
                    [-15 -15 -15 -15 2 2 2 2 1], 7, 7);
expectedJc = sparse([2 2 2], 5:7, -1, 7, 7);
expectedEmb = {0, [1, 4:6], 2, 3};
[h0, J0, Jc, emb] = sapiEmbedProblem(h, J, embeddings, A, false, ...
                                     true, hRange, JRange);
assertEqual(h0, expectedh0)
assertEqual(full(J0), full(expectedJ0))
assertEqual(full(Jc), full(expectedJc))
emb = cellfun(@sort, emb, 'UniformOutput', false);
assertEqual(emb, expectedEmb)
end
