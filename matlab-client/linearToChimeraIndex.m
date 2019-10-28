% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [i, j, u, k] = linearToChimeraIndex(lin, m, n, t)
%% LINEARTOCHIMERAINDEX Convert linear indices to Chimera indices.
%
%  [i j u k] = linearToChimeraIndex(lin, m, n, t)
%
%  The Chimera graph is an m-by-n grid of K_{t, t} cells. Its vertices are
%  (ie. qubits) are naturally indexed by a quadruple (i, j, u, k), where
%  1 <= i <= m, 1 <= j <= n, 1 <= u <= 2, and 1 <= k <= t.  (So (i, j) is
%  the grid coordinate of the cell, u indicates one of the K_{t, t}
%  bipartition halves, and k is the index within the bipartition half.)
%  Vertices/qubits are indexed linearly by sorting these quadruples
%  lexicographically. This function converts linear indices to Chimera
%  index quadruples.
%
%  Input parameters:
%  lin: linear indices.
%  m, n, t: Chimera dimensions.
%
%  Return values:
%  i, j, u, k: Chimera coordinates. These will have the same dimensions
%    as ind.
%
%  See also chimeraToLinearIndex.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if m <= 0
    error('m must be a positive integer');
end

if n <= 0
    error('n must be a positive integer');
end

if t <= 0
    error('t must be a positive integer');
end

if any(lin <= 0) || any(lin > m * n * t * 2)
    error('lin index out of range');
end

[k u j i] = ind2sub([t 2 n m], lin);
