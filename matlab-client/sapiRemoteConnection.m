% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function conn = sapiRemoteConnection(varargin)
%% SAPIREMOTECONNECTION Connect to remote SAPI solvers.
%
%  conn = sapiRemoteConnection(url, token)
%  conn = sapiRemoteConnection(url, token, proxy)
%
%  Input Parameters
%    url: SAPI URL. Get this from the web user interface (using the menu:
%      Developers > Solver API, "Connecting to SAPI" section).
%
%    token: API token. This is also obtained from the web user interface
%      (menu: <user id> > API tokens).
%
%    proxy: Proxy URL. By default (i.e. when no value or an empty matrix is
%      provided), MATLAB's proxy settings are used. If those settings are not
%      available, the environment variables all_proxy, http_proxy, and
%      https_proxy are checked (http_proxy and https_proxy are
%      protocol-specific, all_proxy is not). The environment variable
%      no_proxy is also examined; it can contain a comma-separated list of
%      hosts for which no proxy server should be used, or an asterisk to
%      override the other environment variables. If a proxy URL is given,
%      then it is used directly, without checking MATLAB's settings or any
%      environment variable. In particular, an empty string (i.e. '', which
%      is interpreted differently from []) forces no proxy server to be used.
%
%  Output
%    conn: handle to the remote connection. Do not rely on its internal
%      structure, it may change in future versions.
%
%  See also: sapiLocalConnection, sapiSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

remoteConnection = sapiremote_connection(varargin{:});
solvers = sapiremote_solvers(remoteConnection);
numSolvers = length(solvers);
solverNames = cell(1, numSolvers);
for i = 1 : numSolvers
    solverNames{i} = solvers(i).id;
end

conn = struct('solver_names', {solverNames}, ...
              'get_solver', @(solverName)getSolver(solverName, solvers));

end

function solver = getSolver(solverName, solvers)
ii = find(arrayfun(@(s) strcmp(s.id, solverName), solvers), 1);
solver = makeSolver(solvers(ii));
end

function answer = solve(solver, problemType, problem, params)
if strcmp(problemType, 'ising') || strcmp(problemType, 'qubo')
    problem = sapiremote_encodeqp(solver, problem);
end
answer = sapiremote_solve(solver, problemType, problem, params);
if strcmp(problemType, 'ising') || strcmp(problemType, 'qubo')
    answer = sapiremote_decodeqp(problemType, answer{2});
end
end

function sp = submit(solver, problemType, problem, params)
if strcmp(problemType, 'ising') || strcmp(problemType, 'qubo')
    problem = sapiremote_encodeqp(solver, problem);
end
sp = struct( ...
    'type', 'remote', ...
    'handle', sapiremote_submit(solver, problemType, problem, params));
end

function solver = makeSolver(rsolver)
solver = struct( ...
    'property', rsolver.properties, ...
    'solve', @(problemType, problem, params) solve(rsolver, problemType, problem, params), ...
    'submit', @(problemType, problem, params) submit(rsolver, problemType, problem, params));
end
