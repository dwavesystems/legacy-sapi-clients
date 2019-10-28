% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function answer = sapiSolveQubo(solver, Q, varargin)
%% SAPISOLVEQUBO Solve a QUBO problem.
%
%  answer = sapiSolveQubo(solver, Q)
%  answer = sapiSolveQubo(solver, Q, params, ...)
%  answer = sapiSolveQubo(solver, Q, 'paramName', value, ...)
%
%  Input parameters
%    solver: the solver handle.
%
%    Q: QUBO problem terms. Must be a real-valued matrix of
%      double-precision values. May be either full or sparse.
%
%    params: structure of solver parameters.
%    'paramName', value: individual solver parameters.
%
%  Solver parameters may be given as arbitrary combinations of params
%  structures and 'paramName'/value pairs. Any 'paramName'/value pairs
%  must not be split (i.e. 'paramName' and value must appear
%  consecutively). Duplicate parameter names are allowed but only the
%  last value will be used.
%
%  Output
%    answer: answer structure. Fields:
%      solutions: matrix of solutions, each column is one solution. Values
%        are either 0, 1, or 3. The value 3 indicates an unused variable.
%
%      energies: row vector of solution energies. Entry n is the energy of
%        solution answer.solutions(:, n).
%
%      num_occurrences: row vector of solution counts. This field might
%        not be present depending on the particular solver or solver
%        parameters used. Entry n is the number of times solution
%        answer.solutions(:,n) appeared.
%
%      timing: structure of timing information. Fields are
%        solver-dependent.
%
%  See also sapiSolver, sapiSolveIsing, sapiAsyncSolveIsing,
%  sapiAsyncSolveQubo

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% need to check Q be vector/matrix

params = sapi_solveParams(varargin);

checkSolverParams(solver, params);

sz = length(Q);
problem = sparse(sz, sz);
problem(1 : size(Q, 1), 1 : size(Q, 2)) = Q;

answer = solver.solve('qubo', problem, params);

end
