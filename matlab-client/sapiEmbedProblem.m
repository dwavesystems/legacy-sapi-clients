% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [h0, J0, Jc, embeddings] = ...
  sapiEmbedProblem(h, J, embeddings, A, clean, smear, hRange, JRange)
%sapiEmbedProblem Embed an Ising problem into a given graph
%
%  [h0, J0, Jc] = sapiEmbedProblem(h, J, embeddings, A, clean, ...
%                                  smear, hRange, JRange)
%
%  Input Parameters
%    h: vector of linear Ising problem coefficients.
%
%    J: matrix of quadratic Ising problem coefficients.
%
%    embeddings: cell array of physical variables ("chains") representing
%      each logical variable. That is, logical variable v is represented by
%      physical variables embeddings{v}. Entries must be pairwise disjoint
%      and each chain must induce a connected graph. Physical variables are
%      0-indexed (like the 'qubits' solver property).
%
%    A: adjacency matrix of physical variables. Variables m and n are adjacent
%      if either A(m+1, n+1) or A(n+1, m+1) is nonzero.
%
%    clean: logical value indicating whether or not to clean the embedding:
%      iteratively removing any physical variables from each chain that are:
%        * adjacent to a single variable in the same chain
%        * not adjacent to any variables in other chains
%      Optional, default=false.
%
%    smear: logical value indicating whether or not to smear the embedding:
%      attempt to increase chain sizes so that the scale of h values
%      (relative to hRange) does not exceed the scale of J values (relative
%      to JRange). Smearing is performed after cleaning, so enabled both is
%      potentially useful.
%      Optional, default=false.
%
%    hRange, JRange: valid ranges of h and J values, respectively. Each is a
%      two-element array of the form [min, max] (just like 'h_range' and
%      'j_range' solver properties, which is the likely source for these
%      parameters). These values are only used when 'smear' is true. There is
%      no actual enforcement of h or J values in this function.  Both are
%      optional with default value [-1, 1].
%
%  Output:
%
%     h0, J0: embedded Ising problem.
%
%     Jc: Strong output variable couplings. Jc(ii, jj) == -1 precisely when
%       ii < jj (Jc is upper triangular), A(ii, jj) | A(jj, ii) == true,
%       and ii and jj correspond to the same problem variable.
%
%       The reason that Jc is returned in addition to J0 is that Ising
%       problems have no constraints. A problem variable may map to multiple
%       output variable, so there is a real possibility that the optimal
%       solution of the output problem has inconsistent values for those
%       output variables. In this case, h0 and J0 can be scaled down relative
%       to Jc to produce a new Ising problem (h1, J1):
%
%         h1 = s * h0;  J1 = s * J0 + Jc;
%
%       for some 0 < s < 1. Finding the right value for s is hard. It's easy
%       to find a tiny value that will make every consistent solution better
%       than any inconsistent solution, but this may raise the precision
%       requirements beyond the capabilities of the quantum processor. Also,
%       you may need only the best one or two consistent solutions, allowing
%       a larger value of s. Bottom line: try s = 1 first and decrease until
%       desired consistency is achieved.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if nargin < 5 || isempty(clean)
  clean = false;
end
if nargin < 6 || isempty(smear)
  smear = false;
end
if nargin < 7 || isempty(hRange)
  hRange = [-1 1];
end
if nargin < 8 || isempty(JRange)
  JRange = [-1 1];
end

numLVars = numel(embeddings);

if numel(h) > numLVars
  error('sapi:BadArg', 'size of h must not exceed that of embeddings')
elseif numel(h) < numLVars
  h(numLVars) = 0;
end
h = h(:);

if any(size(J) > numLVars)
  error('sapi:BadArg', 'size of J must not exceed that of embeddings')
elseif any(size(J) < numLVars)
  J(numLVars, numLVars) = 0;
end
J = J + J';

numPVars = length(A);
if any(size(A) < numPVars)
  A(numPVars, numPVars) = 0;
end
A = A | A';
Aup = triu(A, 1);
aVars = any(A);

embeddings = cellfun(@(x) x + 1, embeddings, 'UniformOutput', false);

if clean
    embeddings = cleanEmbedding(h, J, embeddings, A);
end
if smear
    embeddings = smearEmbedding(h, J, embeddings, A, hRange, JRange);
end

h0 = zeros(numPVars, 1);
J0 = zeros(numPVars);
Jc = zeros(numPVars);

usedVars = false(1, numPVars);
for ii=1:numLVars
  emb = embeddings{ii};
  if any(usedVars(emb))
    error('sapi:BadArg', 'Embeddings are not pairwise disjoint')
  end
  usedVars(emb) = true;
  if ~all(aVars(emb))
    error('sapi:BadArg', ...
      'Logical variable %d uses invalid physical variables', ii)
  end
  if ~areConnected(emb, A)
    error('sapi:BadArg', ...
      'Logical variable %d does not induce a connected graph', ii)
  end
  h0(emb) = h(ii) / numel(emb);
  Jc(emb, emb) = -1;
end
Jc = Jc .* Aup;

[ji jj jv] = find(triu(J, 1));
for e=[ji jj jv]'
  split = sum(reshape(A(embeddings{e(1)}, embeddings{e(2)}), 1, []));
  if split == 0
    error('sapi:BadArg', ...
      'No physical edges between logical variables %d and %d', e(1), e(2))
  end
  J0(embeddings{e(1)}, embeddings{e(2)}) = e(3) / split;
  J0(embeddings{e(2)}, embeddings{e(1)}) = e(3) / split;
end
J0 = J0 .* Aup;

embeddings = cellfun(@(x) x - 1, embeddings, 'UniformOutput', false);
end


function newEmb = cleanEmbedding(h, J, vEmb, A)

%when h or J are zero
unusedVars = find(~or (logical(h(:)), any(J+J', 2)));
for ii = unusedVars(:)';
  %allow Embeddings to point to first element
  vEmb{ii} = vEmb{ii}(1);
end

newEmb = cell(1, length(vEmb));
for ii = 1:length(vEmb)
  emb = vEmb{ii};
  nghbrs = setdiff(find(logical(J(:, ii))' + logical(J(ii, :))), ii);
  embNghbr = cell2mat(vEmb(nghbrs));
  finished = false;
  while ~finished && length(emb) > 1
    deg = sum(A(union(embNghbr, emb), emb));
    emb = emb(deg > 1);
    if (all(deg > 1))
      finished = true;
    end
  end
  if isempty(emb)
    emb = vEmb{ii}(1);
  end
  newEmb{ii} = emb;
end
end


function embeddings = smearEmbedding(h, J, embeddings, A, hRange, JRange)

if hRange(1) >= 0 || hRange(2) <= 0
  error('sapi:BadArg', 'hRange must contain positive and negative values')
end
if JRange(1) >= 0 || JRange(2) <= 0
  error('sapi:BadArg', 'JRange must contain positive and negative values')
end

[jr, jc] = find(triu(J, 1));
JMult = arrayfun(@(r, c) nnz(A(embeddings{r}, embeddings{c})), jr, jc);

jI = sub2ind(size(J), jr, jc);
jbound = reshape(JRange((J(jI) > 0) + 1), [], 1);
JScale = min(jbound .* JMult ./ J(jI));

embSizes = cellfun(@numel, embeddings(:));
hbound = reshape(hRange((h > 0) + 1), [], 1);
hScales = hbound .* embSizes ./ h;
hScales(h == 0) = inf;
[~, his] = sort(hScales);

for emb = embeddings(:)'
  A(:, emb{1}) = false;
end

for hi = his(:)'
  if hScales(hi) >= JScale
    break
  end
  targetSize = ceil(JScale * h(hi) / hbound(hi));
  emb = embeddings{hi}(:)';
  ei = numel(emb);
  emb(targetSize) = 0;
  avail = [];
  while ei < targetSize
    if avail
      ei = ei + 1;
      emb(ei) = avail(end);
      A(:, avail(end)) = false;
      avail(end) = [];
    else
      avail = find(any(A(emb(1:ei), :), 1));
      if isempty(avail)
        break
      end
    end
  end
  embeddings{hi} = emb(1:ei);
end
end


function z = areConnected(vs, A)

A = A(vs, vs);
A = A | A';
n = size(A, 1);

seen = [true false(1, n - 1)];
queue = [1 zeros(1, n - 1)];
qStart = 1;
qEnd = 2;

while qStart < qEnd && qEnd <= n
  v = queue(qStart);
  qStart = qStart + 1;

  adj = A(v, :) & ~seen;
  seen = seen | adj;
  adjI = find(adj);
  queue(qEnd : qEnd + length(adjI) - 1) = adjI;
  qEnd = qEnd + length(adjI);
end

z = all(seen);
end
