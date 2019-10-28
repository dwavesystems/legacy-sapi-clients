% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function A = getHardwareAdjacency(solver)
%% GETHARDWAREADJACENCY Hardware graph adjacency matrix.
%
%  A = getHardwareAdjacency(solver)
%
%  Input parameter:
%  solver: handle to a sapi solver.
%
%  Return value:
%  A: adjacency matrix of the hardware graph. Sparse and symmetric.
%
%  See also getChimeraAdjacency.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

properties = sapiSolverProperties(solver);

edges = properties.couplers + 1;  qubits = properties.num_qubits;
A = sparse([edges(1, :) edges(2, :)], [edges(2, :) edges(1, :)], 1, qubits, qubits);
end
