% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function solveQSageExample

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% use a local solver
conn = sapiLocalConnection;
solver = sapiSolver(conn, 'c4-sw_sample');

% objective function is a MATLAB function
numVars = 12;
[bestSolution bestEnergy info] = sapiSolveQSage(@objFunction, numVars, solver, struct('num_reads', 100), struct('verbose', 2));
disp('bestSolution:');
disp(bestSolution);
disp('bestEnergy:');
disp(bestEnergy);
disp('info:');
disp(info);

% objective function is random Ising problem
solverProperty = sapiSolverProperties(solver);
% make a random Ising problem as the optimization objective
numVar = solverProperty.num_qubits;
h = randi(3, numVar, 1) - 2;
J = triu(randi(5, numVar) - 3, 1);
% construct the objective function
G = @(S) dot(S, J * S, 1) + h' * S;
[bestSolution bestEnergy info] = sapiSolveQSage(G, numVar, solver, struct(), struct('timeout', 20, 'verbose', 2));
disp('bestSolution:');
disp(bestSolution);
disp('bestEnergy:');
disp(bestEnergy);
disp('info:');
disp(info);

end

function answer = objFunction(states)

% Each column in the states is one state
% The return value is the objective values for each column

m = size(states, 1);
%n = size(states, 2)
answer = sum(states(1 : floor(m / 2), :), 1) - sum(states(floor(m / 2) + 1 : end, :), 1);

end
