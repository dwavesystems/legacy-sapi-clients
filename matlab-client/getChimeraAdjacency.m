% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function A = getChimeraAdjacency(m, n, t)
%% GETCHIMERAADJACENCY Chimera graph adjacency matrix.
%
%  A = getChimeraAdjacency(m, n, t)
%
%  Build the adjacency matrix A for the Chimera architecture. The
%  architecture is an m-by-n lattice of elements where each element is a
%  K_{t, t} bipartite graph. In this representation the rows and columns of
%  the adjacency matrix A are indexed by (i, j, u, k) where
%    * 1 <= i <= m gives the row in the lattice
%    * 1 <= j <= n gives the column in the lattice
%    * 1 <= u <= 2 labels which of the two partitions
%    * 1 <= k <= t labels one of the t elements in the partition
%  Matrix elements of A are explicitly given by
%
%    A_{i, j; u; k | i', j'; u'; k'} =
%      [i == i'] [j == j'] [|u - u'| == 1] +             (bipartite connections)
%      [|i - i'| == 1] [j == j'] [u == u' == 1] [k == k'] +  (row connections)
%      [i == i'] [|j - j'| == 1] [u == u' == 2] [k == k']    (col connections)
%
%    where [p] = 1 if p is true and [p] = 0 if p is false
%
%  Input parameters:
%  m, n, t: Chimera dimensions. t defaults to 4, n defaults to m.
%
%  Return value:
%  A: adjacency matrix.
%
%  See also getHardwareAdjacency.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if nargin < 3
   t = 4;
end
if nargin < 2
   n = m;
end

if m <= 0
    error('m must be a positive integer');
end

if n <= 0
    error('n must be a positive integer');
end

if t <= 0
    error('k must be a positive integer');
end

tRow = spones(diag(ones(1, m - 1), 1) + diag(ones(1, m - 1), -1));                % [|i - i'| = 1]
tCol = spones(diag(ones(1, n - 1),1) + diag(ones(1, n - 1), -1));                % [|j - j'| = 1]
A = kron(speye(m * n), kron(spones([0 1; 1 0]), ones(t))) + ...             % bipartite connections
   kron(kron(tRow, speye(n)), kron(spones([1 0; 0 0]), speye(t))) +  ...  % connections between rows
   kron(kron(speye(m), tCol), kron(spones([0 0; 0 1]), speye(t)));        % connections between cols

