% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function sols = sapiUnembedAnswer(sols, embeddings, brokenChains, h, J)
%sapiUnembedAnswer "Unembed" answer of an embedded Ising problem
%
%  sols = sapiUnembedAnswer(sols, embeddings, brokenChains)
%  sols = sapiUnembedAnswer(sols, embeddings, 'minimize_energy', h, J)
%
%  Input parameters:
%    sols: solution bits. Each column is a single solution. This is the format
%      of the 'solutions' field in the answer returned by sapiSolveIsing. Any
%      rows indexed by embeddings are expected to contain only +1 or -1
%      values. This expectation is not validated; violating it can cause
%      strange results.
%
%    embeddings: cell array of physical variables representing each logical
%      variable. Use the value returned by sapiEmbedProblem (which can differ
%      from the value passed to sapiEmbedProblem if the 'smear' or 'clean'
%      options are enabled.
%
%    brokenChains: strategy for repairing broken chains. If every value of
%      the chain sols(embeddings{ii}, jj) is the same, that common value will
%      be used in the unembedded answer. If not, there are multiple options
%      to repair the chain:
%        'discard': do not attempt to repair the chain; simply discard the
%          solution (column).
%        'vote': choose whichever of +1 and -1 appears more often in the
%          solution. Ties are broken randomly.
%        'weighted_random': choose +1 probability equal to the fraction of
%          bits in the solution equal to +1.
%        'minimize_energy': greedy heuristic that repairs chains one at a
%          time, choosing the value at each step that minimizes the energy
%          over intact/repaired chains. Requires h, J parameters to be
%          provided.
%
%    h, J: original Ising problem before embedding, same as passed to
%      sapiEmbedProblem. Required for the 'minimize_energy' brokenChains
%      option, ignored otherwise.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

embeddings = cellfun(@(x) x + 1, embeddings(:), 'UniformOutput', false);

switch brokenChains
  case 'minimize_energy'
    sols = unembedMinimizeEnergy(sols, embeddings, h, J);
  case 'vote'
    sols = unembedVote(sols, embeddings);
  case 'discard'
    sols = unembedDiscard(sols, embeddings);
  case 'weighted_random'
    sols = unembedWeightedRandom(sols, embeddings);
  otherwise
    error('Invalid ''broken_chains'' argument')
end
end


function outSols = unembedMinimizeEnergy(sols, embeddings, h, J)
numEmbs = numel(embeddings);
if length(h) < numEmbs
  h(numEmbs) = 0;
end
if any(size(J) < numEmbs)
  J(numEmbs, numEmbs) = 0;
end
h = h(:);
J = J + J';

e1 = cellfun(@(emb) emb(1), embeddings);
numSols = size(sols, 2);
outSols = zeros(numEmbs, numSols);
for ii=1:numSols
  bad = cellfun(@(emb) any(sols(emb, ii) ~= sols(emb(1), ii)), ...
                embeddings);
  outSols(~bad, ii) = sols(e1(~bad), ii);
  badI = find(bad);
  badE = h(badI) + J(badI, :) * outSols(:, ii);
  while ~isempty(badI)
    [~, bii] = max(abs(badE));
    vi = badI(bii);
    s = -sign(badE(bii));
    if s == 0
      s = 1;
    end
    outSols(vi, ii) = s;
    badI(bii) = badI(end);
    badI = badI(1:end-1);
    badE(bii) = badE(end);
    badE = badE(1:end-1);
    if ~isempty(badE)
      badE = badE + s * J(badI, vi);
    end
  end
end
end


function outSols = unembedVote(sols, embeddings)
outSols = zeros(numel(embeddings), size(sols, 2));
for ii=1:numel(embeddings)
  es = sols(embeddings{ii}, :);
  os = sign(sum(es, 1));
  os(os == 0) = (2 * randi([0 1], 1, sum(os == 0)) - 1);
  outSols(ii, :) = os';
end
end

function outSols = unembedDiscard(sols, embeddings)
e1 = cellfun(@(emb) emb(1), embeddings);
keep = true(1, size(sols, 2));
for emb = embeddings(:)'
  keep = keep & all(bsxfun(@eq, sols(emb{1}(1), :), sols(emb{1}, :)));
end
outSols = sols(e1, keep);
end

function outSols = unembedWeightedRandom(sols, embeddings)
numSols = size(sols, 2);
outSols = zeros(numel(embeddings), numSols);
for ii=1:numel(embeddings)
  es = sols(embeddings{ii}, :);
  p = sum(es == 1, 1) / numel(embeddings{ii});
  outSols(ii, :) = 2 * (rand(1, numSols) < p) - 1;
end
end
