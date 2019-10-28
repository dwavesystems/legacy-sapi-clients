% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [qTerms varRepl] = reduceDegree(terms)
%% REDUCEDEGREE Reduce the degree of a set of objectives specified by terms
%
%  [qTerms varRepl] = reduceDegree(terms)
%
%  Reduce the degree of a set of objectives specified by terms to have
%  maximum two degrees via the introduction of ancillary variables.
%
%  Input parameter:
%  terms: Each term's variables in the expression
%
%  Return values:
%  qTerms: Terms after using ancillary variables
%  varRepl: Ancillary variables
%
%  See also makeQuadratic.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

terms = terms(:);
assert(all(cellfun(@isvector, terms)))

termSizes = cellfun(@length, terms);
bigTerms = termSizes > 2;

qTerms = cell(1,length(terms));
qTerms(~bigTerms) = terms(~bigTerms);

maxVar = max(cellfun(@max, terms));

pairsCell = cellfun(@getPairs, terms, 'UniformOutput', false);
pairs = zeros(sum(cellfun(@(p) size(p,1), pairsCell)), 2);
termIndices = zeros(size(pairs,2), 1);

j = 1;
for i = 1 : numel(pairsCell)
  len = length(pairsCell{i});
  if len > 0
    pairs(j : j + len - 1, :) = pairsCell{i};
  end
  termIndices(j : j + len - 1) = i;
  j = j + len;
end

[pairs pairsSortI] = sortrows(pairs);
termIndices = termIndices(pairsSortI);

varReplSize = 2 * max(termSizes);
varReplI = 0;
varRepl = zeros(3, varReplSize);

newVar = maxVar;

while any(bigTerms)

  newVar = newVar + 1;

  % count occurences
  uniquePairs = [true; any(diff(pairs), 2)];
  uniquePairsI = find(uniquePairs);
  numOccurrences = [ uniquePairsI(2 : end); size(pairs, 1) + 1 ] - uniquePairsI;

  % find most frequent pair
  [ maxOccurrences maxOccI ] = max(numOccurrences);
  pairI1 = uniquePairsI(maxOccI);
  pairI = pairI1 : pairI1 + maxOccurrences - 1;
  pair = pairs(pairI1, :);
  pairTerms = termIndices(pairI);

  % update variable replacement table
  varReplI = varReplI + 1;
  if varReplI > varReplSize
    varRepl = [ varRepl zeros(3, varReplSize) ]; %#ok<AGROW>
    varReplSize = 2 * varReplSize;
  end

  varRepl(:, varReplI) = [ newVar pairs(pairI1, :) ];

  % calculate new rows to be added to allPairs:
  % grab rows of allPairs containing pair(1) (but not pair(2) to avoid
  % duplicates and replaced pairs) and in pairTerms.
  % Also update termSizes and bigTerms.  Don't add a new pairs that
  % aren't part of big terms.
  pairTermsZ = false(numel(terms), 1);
  pairTermsZ(pairTerms) = true;
  termRowsZ = pairTermsZ(termIndices);
  termRowsI = find(termRowsZ);
  termPairs = pairs(termRowsI, :);
  termTermIndices = termIndices(termRowsI);

  termRowsP2Z = termPairs(:, 1) == pair(2) | termPairs(:, 2) == pair(2);

  termSizes(pairTerms) = termSizes(pairTerms) - 1;
  bigTerms = termSizes > 2;

  termRowsP1C1Z = termPairs(:, 1) == pair(1) & ~termRowsP2Z;
  termRowsP1C2Z = termPairs(:, 2) == pair(1) & ~termRowsP2Z;

  bigTermRowsP1C1Z = termRowsP1C1Z & bigTerms(termTermIndices);
  bigTermRowsP1C2Z = termRowsP1C2Z & bigTerms(termTermIndices);
  bigTermRowsP1Z = bigTermRowsP1C1Z | bigTermRowsP1C2Z;

  newPairs = [newVar * ones(sum(bigTermRowsP1Z), 1) ...
    [ termPairs(bigTermRowsP1C1Z, 2); termPairs(bigTermRowsP1C2Z, 1) ]];
  newTermIndices = [ termTermIndices(bigTermRowsP1C1Z); ...
    termTermIndices(bigTermRowsP1C2Z) ];

  % any pair marked by termRowsP1C1Z but not bigTermRowsP1C1Z (same with
  % termRowsP1C2Z, bigTermRowsP1C2Z) is the sole surviving pair of the
  % corresponding term.  Save it in qTerms
  qTermsRowI = termRowsI(termRowsP1C1Z & ~bigTermRowsP1C1Z);
  for i = qTermsRowI(:)'
    qTerms{termIndices(i)} = [newVar pairs(i, 2)];
  end

  qTermsRowI = termRowsI(termRowsP1C2Z & ~bigTermRowsP1C2Z);
  for i = qTermsRowI(:)'
    qTerms{termIndices(i)} = [newVar pairs(i, 1)];
  end

  % remove rows of allPairs such that:
  % 1. row term is in pairTerms
  % 2. row pair intersects replaced pair
  keepRows = true(size(pairs, 1), 1);
  keepRows(termRowsI) = ~(termRowsP1C1Z | termRowsP1C2Z | termRowsP2Z);

  % update!
  pairs = [ pairs(keepRows, :); newPairs ];
  termIndices = [ termIndices(keepRows); newTermIndices ];

end

varRepl = varRepl(:, 1 : varReplI);

end

function p = getPairs(x)

n = length(x);

if n > 2
  [k(:, 1) k(:, 2)] = find(triu(ones(n), 1));
  p = x(k);
else
  p = [];
end

end
