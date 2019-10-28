% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function ret = chimeraToLinearIndex(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
%% CHIMERATOLINEARINDEX Convert Chimera indices to linear indices.
%
%  ind = chimeraToLinearIndex(i, j, u, k, m, n, t)
%  ind = chimeraToLinearIndex(idx, m, n, t)
%
%  The Chimera graph is an m-by-n grid of K_{t, t} cells. Its vertices are
%  (ie. qubits) are naturally indexed by a quadruple (i, j, u, k), where
%  1 <= i <= m, 1 <= j <= n, 1 <= u <= 2, and 1 <= k <= t.  (So (i, j) is
%  the grid coordinate of the cell, u indicates one of the K_{t, t}
%  bipartition halves, and k is the index within the bipartition half.)
%  Vertices/qubits are indexed linearly by sorting these quadruples
%  lexicographically. This function converts Chimear index quadruples to
%  linear indices.
%
%  Input parameters:
%  i, j, u, k: Chimera coordinates. These must be column vectors (scalars
%    count) of equal lengths.
%  idx: equivalent to [i j u k].
%  m, n, t: Chimera dimensions.
%
%  Return value:
%  ind: linear indices of the Chimera coordinates.
%
%  See also linearToChimeraIndex.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if nargin == 7
    num = min([length(arg1) length(arg2) length(arg3) length(arg4)]);
    idx = [arg1(1 : num) arg2(1 : num) arg3(1 : num) arg4(1 : num)];
    m = arg5;  n = arg6;  t = arg7;
elseif nargin == 4
    idx = arg1;  m = arg2;  n = arg3;  t = arg4;  num = size(idx, 1);
else
    error('Need seven or four inputs');
end;

if m <= 0
    error('m must be a positive integer');
end

if n <= 0
    error('n must be a positive integer');
end

if t <= 0
    error('t must be a positive integer');
end

multipliers = repmat([2 * t * n 2 * t t 1], num, 1);
if any(idx(:, 1) > m | idx(:, 1) < 1)
   error('i index out of range');
end;
if any(idx(:, 2) > n | idx(:, 2) < 1)
   error('j index out of range');
end;
if any(idx(:, 3) > 2 | idx(:, 3) < 1)
   error('u index out of range');
end;
if any(idx(:, 4) > t | idx(:, 4) < 1)
   error('k index out of range');
end;
ret = 1 + sum((idx - 1) .* multipliers, 2);
