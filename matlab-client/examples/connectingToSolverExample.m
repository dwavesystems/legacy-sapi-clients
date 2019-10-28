% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

% This example shows how display remote solver properties as well as to use the solver to
% asynchronously solve an ising problem by using sapi_asyncSolveIsing function. It also shows how
% to cancel a submitted problem by using sapi_cancelSubmittedProblem function.

% usage:
%  1. list all solver properties of remote connection and solve problem:
%     sapi_connectingToSolver url token solver_name

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

function connectingToSolverExample(url, token, solver_name)

% using a remote solver
% Create a remote SAPI connection handle
remoteConnection = sapiRemoteConnection(url, token);

% List solvers' names
solverNames = sapiListSolvers(remoteConnection);
disp('solvers'' names:');
disp(solverNames);

% Create a SAPI solver handle
remoteSolver = sapiSolver(remoteConnection, solver_name);

% Retrieve solver properties from a SAPI solver handle
props = sapiSolverProperties(remoteSolver);
disp('solver''s properties:');
disp(props);

% Define h1 - load a single available h value with 1
h1 = [];
h1(props.qubits(1) + 1) = 1;

% Row and column indices
rowIdx = props.couplers(1, :) + 1;
colIdx = props.couplers(2, :) + 1;

% Create J1 matrix of all -1
J1 = sparse(rowIdx, colIdx, -1, props.num_qubits, props.num_qubits);

% Create J2 matrix
J2 = sparse(rowIdx, colIdx, 1, props.num_qubits, props.num_qubits);

% Use sapiAwaitSubmission and sapiAwaitCompletion to solve two Ising problems asynchronously
submittedProblem{1} = sapiAsyncSolveIsing(remoteSolver, h1, J1, 'num_reads', 10);
submittedProblem{2} = sapiAsyncSolveIsing(remoteSolver, [], J2, 'num_reads', 20, 'postprocess', 'sampling', 'num_spin_reversal_transforms', 1);

timeout = 300.0;
newSubmittedProblem = sapiAwaitSubmission(submittedProblem, timeout);

minDone = 1;
sapiAwaitCompletion(newSubmittedProblem, minDone, timeout);

for k=1:length(newSubmittedProblem)
    if sapiAsyncDone(newSubmittedProblem{k})
        try
            answer = sapiAsyncResult(newSubmittedProblem{k});
            fprintf('Ising answer %d:\n', k);
            disp(answer);
        catch me
            fprintf('Error: %s\n', me.message);
        end
    else
        sapiCancelSubmittedProblem(newSubmittedProblem{k});
        fprintf('Problem %d cancelled\n', k);
    end
end

end
