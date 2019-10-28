% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [Q, quboOffset] = isingToQubo(h, J)
%isingToQubo Convert an Ising problem to a QUBO
%
%  [Q, quboOffset] = isingToQubo(h, J)
%
%  Input parameters:
%  h: linear Ising coefficients.
%  J: quadratic coefficients.
%
%  Return value:
%  Q: qubo matrix. Linear coefficients appear on the diagonal.
%  quboOffset: Offset from QUBO objective value to Ising objective for
%    equivalent states. In particular, if s is any {+1, -1}-vector and
%    x = (1 + s) / 2, then:
%
%      x' * Q * x + quboOffset == s' * J * s + h' * s

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if any(diag(J))
    error('J cannot have nonzero diagonal elements');
end

if isempty(h)
  h = [];
end
if isempty(J)
  J = [];
end

dim = max(length(h), length(J));
if length(h) < dim
    h(dim) = 0;
end
if any(size(J) < dim)
    J(dim, dim) = 0;
end

Q = 4 * J + 2 * spdiags(h(:) - sum(J + J', 2), 0, dim, dim);
quboOffset = full(sum(J(:)) - sum(h(:)));

end
