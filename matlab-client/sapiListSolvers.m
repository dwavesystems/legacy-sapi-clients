% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function solvers = sapiListSolvers(connection)
%% SAPILISTSOLVERS List solvers available from a connection.
%
%  sapiListSolvers(connection)
%  solvers = sapiListSolvers(connection)
%
%  Input Parameters
%    connection: a connection handle, returned by either
%      sapiLocalConnection or sapiRemoteConnection.
%
%  Output
%    solvers: a cell array of solver names. If no output is specified, the
%      list of solver names is printed unquoted, one solver per line (for
%      ease of copying).
%
%  Solver names are used to acquire solver handles with the sapiSolver
%  function.
%
%  See also: sapiLocalConnection, sapiRemoteConnection, sapiSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

s = connection.solver_names;
if nargout == 0
  for ii=1:numel(s)
    fprintf('%s\n', s{ii})
  end
else
  solvers = s;
end
end

