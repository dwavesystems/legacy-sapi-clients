% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function submittedProblem = sapiAsyncSolveIsing(solver, h, J, varargin)
%% SAPIASYNCSOLVEISING Solve an Ising problem asynchronously.
%
%  submittedProblem = sapiAsyncSolveIsing(solver, h, J)
%  submittedProblem = sapiAsyncSolveIsing(solver, h, J, params, ...)
%  submittedProblem = sapiAsyncSolveIsing(solver, h, J, 'paramName', value, ...)
%
%  Input parameters
%    solver: the solver handle.
%
%    h: linear Ising problem terms. Must be a real-valued vector of
%      double-precision values. May be either full or sparse.
%
%    J: quadratic Ising problem terms. Must be a real-valued matrix of
%      double-precision values. Diagonal entries must be zero.  May be
%      either full or sparse.
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
%    submittedProblem: a handle to the result of the submitted problem. Use
%      sapiAsyncDone to determine when the problem is complete and
%      sapiAsyncResult to retrieve the answer. Use
%      sapiCancelSubmittedProblem to cancel the submitted problem.
%
%  This function works with both local and remote solvers. If a local
%  solver is given, the problem is solved lazily: all calls to
%  sapiAsyncDone will return true, and the problem will actually be solved
%  during the call to sapiAsyncResult.
%
%  See also sapiSolver, sapiSolveIsing, sapiSolveQubo, sapiAsyncSolveQubo,
%  sapiAsyncDone, sapiAsyncResult, sapiCancelSubmittedProblem,
%  sapiAwaitCompletion, sapiAwaitSubmission

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% need to check h be vector, J be vector/matrix

if any(diag(J))
    error('J''s diagonal entries must be zero');
end

params = sapi_solveParams(varargin);

checkSolverParams(solver, params);

sz = max(length(h), length(J));
problem = sparse(sz, sz);
problem(1 : size(J, 1), 1 : size(J, 2)) = J;
if ~isempty(h)
    problem(1 : sz + 1 : sz * length(h)) = h;
end
submittedProblem = solver.submit('ising', problem, params);

end
