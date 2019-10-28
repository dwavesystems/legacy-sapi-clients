% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [h, J, isingOffset] = quboToIsing(Q)
%quboToIsing Convert a QUBO to an Ising problem
%
%  [h J isingOffSet] = quboToIsing(Q)
%
%  Input parameter:
%  Q: QUBO matrix.
%
%  Return values:
%  h: linear Ising coefficients.
%  J: quadratic Ising coefficients.
%  isingOffset: Offset from Ising objective value to QUBO objective for
%    equivalent states. In particular, if x is any {0, 1}-vector and
%    s = 2 * x - 1, then:
%
%      s' * J * s + h' * s + isingOffset == x' * Q * x

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if isempty(Q)
    h = [];
    J = [];
    isingOffset = 0;
    return;
end

dim = length(Q);
if any(size(Q) < dim)
    Q(dim, dim) = 0;
end

q = diag(Q);
isingOffset = full((sum(Q(:)) + sum(q)) / 4);
Q = Q - diag(q);
J = Q / 4;
h = full(q / 2 + sum(Q + Q', 2) / 4);

end
