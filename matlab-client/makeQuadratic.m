% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [Q, qTerms, varRepl] = makeQuadratic(f, penaltyWeight)
%% MAKEQUADRATIC Represent a function f as a quadratic objective
%
% [Q, qTerms, varRepl] = makeQuadratic(f, penaltyWeight)
%
%  Given a function f defined over binary variables represented as a
%  vector stored in decimal order, e.g. f(000), f(001), f(010), f(011),
%  f(100), f(101), f(110), f(111) represent it as a quadratic objective.
%
%  Input parameter:
%  f: A function f defined over binary variables represented as a
%  vector stored in decimal order
%  penaltyWeight: If this parameter is supplied it sets the strength of the
%  penalty used to define the product constraints on the new ancillary
%  variables. The default value is usually sufficiently large, but may be
%  larger than necessary.
%
%  Return values:
%  Q: Quadratic coefficients
%  qTerms: The terms in the qubo arising from quadraticization of the
%          interactions present in f
%  varRepl: The definition of the new ancillary variables, vars(1,i)
%           represents the product of vars(2,i) and vars(3,i)
%
%  See also reduceDegree.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

%% parameter checking
if isempty(f)
    error('f length must be power of 2');
end

if f(1) ~= 0
    error('makeQuadratic expects f(1), the constant to be zero');
end

if bitand(length(f), length(f) - 1) ~= 0
    error('f length must be power of 2');
end

n = log2(length(f));

%% find coefficents
w_inverse = [1 0; -1 1];  tmp = [1 0; -1 1];
for i = 1 : n - 1
    w_inverse = kron(tmp, w_inverse);
end
c = w_inverse * f;

%% find non-zero coefficients
nonZero = 1 : 2^n;
nonZero = nonZero(abs(c) > 1e-10);

% find interactions using as few new variables as possible
terms = arrayfun(@(i) find(fliplr(dec2bin(i, n) - '0') == 1), 0 : 2^n - 1, 'UniformOutput', false);
[qTerms, varRepl] = reduceDegree(terms(nonZero));
numVars = n + size(varRepl, 2);
Q = zeros(numVars);  r = zeros(numVars,1);

%% add in to the objective
for i = 1 : length(qTerms)
   switch length(qTerms{i})
      case 1
         r(qTerms{i}) = r(qTerms{i}) + c(nonZero(i));
      case 2
         dims = qTerms{i};
         Q(dims(1), dims(2)) = Q(dims(1), dims(2)) + c(nonZero(i)) / 2;
         Q(dims(2), dims(1)) = Q(dims(2), dims(1)) + c(nonZero(i)) / 2;
      otherwise
         error('should only have pairwise interactions');
   end
end

%%  add the penalty terms enforcing pairwise constraints
if nargin < 2
  penaltyWeight = 10 * max(abs(Q(:)));
end
if numVars > n
   QAnd = penaltyWeight * [0 -1 -1; -1 0 1/2; -1 1/2 0];  rAnd = penaltyWeight * [3; 0; 0];
   for i = 1 : size(varRepl, 2)
      dims = varRepl(:, i);
      Q(dims, dims) = Q(dims, dims) + QAnd;  r(dims) = r(dims) + rAnd;
   end
end
Q(1 : numVars + 1 : end) = Q(1 : numVars + 1 : end) + r';
