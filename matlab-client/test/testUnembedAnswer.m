% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testUnembedAnswer %#ok<STOUT,*DEFNU>
initTestSuite
end



function testTrival

assertTrue(isempty(sapiUnembedAnswer([], {}, 'minimize_energy', [], [])))
assertTrue(isempty(sapiUnembedAnswer([], {}, 'vote', [], [])))
assertTrue(isempty(sapiUnembedAnswer([], {}, 'discard', [], [])))
assertTrue(isempty(sapiUnembedAnswer([], {}, 'weighted_random', [], [])))

end



function testDiscard

embeddings = {[2 5 6], 0, [1 3]};
solutions = [+1, +1, +1, +1, +3, +1, +1
             +1, +1, -1, +1, +1, -1, +1
             +1, +1, -1, +1, +1, -1, -1
             +1, -1, -1, +1, +1, -1, -1
             +1, -1, -1, -1, +1, -1, -1
             -1, -1, -1, -1, 33, -1, -1]';

expected = [+1, +1, +1
            -1, +1, +1
            -1, +1, -1
            -1, -1, -1]';

assertEqual(sapiUnembedAnswer(solutions, embeddings, 'discard'), expected)

end



function testDiscardAll

embeddings = {[0 1]};
solutions = [-1 +1; -1 +1; +1 -1]';

assertTrue(isempty(sapiUnembedAnswer(solutions, embeddings, 'discard')))

end



function testVoteNoties

embeddings = {[1 2], [3, 5, 4]};
solutions = [3 +1 +1 -1 -1 -1
             3 -1 -1 -1 +1 +1
             3 -1 -1 +1 +1 -1
             3 +1 +1 -1 +1 -1]';
expected = [+1 -1
            -1 +1
            -1 +1
            +1 -1]';

assertEqual(sapiUnembedAnswer(solutions, embeddings, 'vote'), expected)

end



function testVoteTies

embeddings = {[0 2] [1 4]};
solutions = [+1 +1 -1 3 -1
             -1 +1 +1 3 -1
             +1 -1 -1 3 +1
             -1 -1 +1 3 +1]';

results = arrayfun(@(n) sapiUnembedAnswer(solutions, embeddings, 'vote'), ...
                   1:20, 'UniformOutput', false);
results = cat(3, results{:});
means = mean(results, 3);

assertEqual(size(results, 1), 2)
assertEqual(size(results, 2), 4)
assertTrue(all((-1 < means(:)) & (means(:) < 1)))

end


function testSC198

embeddings = {0, [1 3]};
solutions = [-1 -1 3 -1
              1  1 3  1
              1  1 3  1]';
expected = [-1 -1; 1 1; 1 1]';

assertEqual(sapiUnembedAnswer(solutions, embeddings, 'vote'), expected)

end



function testWeightedRandom
% this test relies on random behaviour! yikes!
% ensure runs and acceptance range are both suitably large
runs = 10000;
embeddings = {[0 3], [4 2 1 5 6 7 10]};
solutions = [+1 -1 -1 +1 -1 -1 -1 -1 3 3 -1
             -1 +1 +1 +1 -1 -1 -1 -1 3 3 -1]';

tot = zeros(2);
for ii=1:runs
  tot = tot + sapiUnembedAnswer(solutions, embeddings, 'weighted_random');
end

assertEqual(tot(:, 1), [runs; -runs])
assertTrue(-400 < tot(1, 2) && tot(1, 2) < 400)
assertTrue(-5100 < tot(2, 2) && tot(2, 2) < -3500)

end



function testSAPI1568
% singleton embeddings cause summing in wrong dimension

embeddings = {0};
solutions = [+1 -1 +1 -1];

assertEqual( ...
    sapiUnembedAnswer(solutions, embeddings, 'weighted_random'), ...
    solutions)

end



function testMinimizeEnergy

embeddings = {[0 5], [1 6], [2 7], [3 8], [4 10]};
h = [];
J = [ 0 -1  2  3 -1
      0  0  0  3 -1
      0 -1  0  1 -2
      0 -1  0  0  1];
solutions = [-1 -1 -1 -1 -1 -1 +1 +1 +1 3 +1
             +1 +1 +1 +1 +1 -1 +1 -1 -1 3 -1
             +1 +1 -1 +1 -1 -1 -1 -1 -1 3 -1]';
expected = [-1 -1 -1 +1 -1
            +1 +1 +1 -1 +1
            -1 -1 -1 +1 -1]';

assertEqual( ...
  sapiUnembedAnswer(solutions, embeddings, 'minimize_energy', h, J), ...
  expected)

end




function testMinimizeEnergyEasy

embeddings = {[0 1], 2, [4 5 6]};
h = -1;
J = [];
solutions = [-1 -1 +1 3 -1 -1 -1
             -1 +1 -1 3 +1 +1 +1]';
expected = [-1 +1 -1; +1 -1 +1]';

assertEqual( ...
  sapiUnembedAnswer(solutions, embeddings, 'minimize_energy', h, J), ...
  expected)

end
