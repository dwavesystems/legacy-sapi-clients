% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testSolveQSageMex %#ok<STOUT,*DEFNU>
initTestSuite
end

function answer = solveIsing(h, J)

answer = struct;
answer.solutions =  2 * (rand(128, 10) < 0.5) - 1;
answer.energies = sum(answer.solutions .* (J * answer.solutions)) + h' * answer.solutions;

end

function answer = objFunction(states)

% Each column in the states is one state 
% The return value is the objective values for each column

m = size(states, 1);
answer = sum(states(1:floor(m/2), :), 1) - sum(states(floor(m/2)+1:end, :), 1);

end

function answer = objFunctionReturnSizeWrong(states)

% Each column in the states is one state 
% The return value is the objective values for each column

m = size(states, 1);
answer = sum(states(1:floor(m/2), :), 1) - sum(states(floor(m/2)+1:end, :), 1);
answer = [answer 1];

end

function answer = objFunctionReturnTypeWrong(states)

answer = 'a';

end

function A = getChimeraAdjacency(m, n, k)

if nargin < 3
   k = 4;
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

if k <= 0
    error('k must be a positive integer');
end

tRow = spones(diag(ones(1, m - 1), 1) + diag(ones(1, m - 1), -1));                % [|i-i'|=1]
tCol = spones(diag(ones(1, n - 1),1) + diag(ones(1, n - 1), -1));                % [|j-j'|=1]
A = kron(speye(m * n), kron(spones([0 1; 1 0]), ones(k))) + ...             % bipartite connections
   kron(kron(tRow, speye(n)), kron(spones([1 0; 0 0]), speye(k))) +  ...  % connections between rows
   kron(kron(speye(m), tCol), kron(spones([0 0; 0 1]), speye(k)));        % connections between cols

end

function answer = lpSolver(f, Aineq, bineq, Aeq, beq, lb, ub) 

answer = ones(1, length(f));

end

function answer = lpSolverReturnSizeWrong(f, Aineq, bineq, Aeq, beq, lb, ub) 

answer = ones(1, length(f) + 1);

end

function answer = lpSolverReturnTypeWrong(f, Aineq, bineq, Aeq, beq, lb, ub) 

answer = 'a';

end

function testSolveQSageMexInvalidParameters

numVars = 20;

isingSolver = struct;
isingSolver.solve_ising = @solveIsing;
isingSolver.qubits = 0 : 127;
adj = getChimeraAdjacency(4, 4, 4);
isingSolver.couplers = zeros(2, nnz(adj) / 2);
index = 1;
for i = 1 : 128
    for j = i + 1 : 128
        if adj(i, j) ~= 0
            isingSolver.couplers(1, index) = i - 1;
            isingSolver.couplers(2, index) = j - 1;
            index = index + 1;
        end
    end
end




% invalid objective function
assertExceptionThrown(@()solve_qsage_mex([], numVars, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex('a', numVars, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(1, numVars, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunctionReturnSizeWrong, numVars, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunctionReturnTypeWrong, numVars, isingSolver, struct()), '');






% numVars is 0
[bestSolution bestEnergy info] = solve_qsage_mex(@objFunction, 0, isingSolver, struct());
assertEqual(isempty(bestSolution), true);
assertEqual(bestEnergy, 0);

% numVars must be an integer >= 0
assertExceptionThrown(@()solve_qsage_mex(@objFunction, -1, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, 'a', isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, '', isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, [], isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, {}, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, [1 1], isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, [1; 1], isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, {1 1}, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, {1; 1}, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, NaN, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, +NaN, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, nan, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, +nan, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, -NaN, isingSolver, struct()), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, -nan, isingSolver, struct()), '');






% solver has no qubits or couplers or solve_ising
invalidIsingSolver = struct;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');
invalidIsingSolver.solve_ising = @solveIsing;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');
invalidIsingSolver.qubits = 0 : 127;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');

invalidIsingSolver = isingSolver;
invalidIsingSolver.h_range = [];
invalidIsingSolver.j_range = [-1 1];
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');
invalidIsingSolver.h_range = [1 -1];
invalidIsingSolver.j_range = [-1 1];
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');

invalidIsingSolver = isingSolver;
invalidIsingSolver.h_range = [-1 1];
invalidIsingSolver.j_range = [];
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');
invalidIsingSolver.h_range = [-1 1];
invalidIsingSolver.j_range = [1 -1];
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, invalidIsingSolver, struct()), '');






% draw_sample must be a boolean
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', 1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('draw_sample', -nan)), '');






% exit_threshold_value must be a number
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('exit_threshold_value', -nan)), '');






% init_solution is not a vector or matrix
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', {1 1 1 1 1 1 1 1 1 1 1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', {{1 1 1 1 1 1 1 1 1 1 1 1}})), '');

% init_solution contains non-integer
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', ['a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'; 'a'])), '');

% init_solution contains integer that is not -1/1 when the isingQubo is "ising"
initSolution = ones(1, numVars);
initSolution(1) = 0;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', initSolution)), '');

% init_solution contains NaN
initSolution = ones(1, numVars);
initSolution(1) = NaN;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', initSolution)), '');

% init_solution's length is not the same as number of variables
initSolution = ones(1, numVars + 1);
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('init_solution', initSolution)), '');

% init_solution contains integer that is not 0/1 when the isingQubo is "qubo"
initSolution = ones(1, numVars);
initSolution(1) = -1;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', 'qubo', 'init_solution', initSolution)), '');

% init_solution contains NaN
initSolution = ones(1, numVars);
initSolution(1) = NaN;
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', 'qubo', 'init_solution', initSolution)), '');

% init_solution's length is not the same as number of variables
initSolution = ones(1, numVars + 1);
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', 'qubo', 'init_solution', initSolution)), '');






% ising_qubo must be a string of 'ising' or 'qubo'
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', -1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', 1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('ising_qubo', -nan)), '');






% invalid lp_solver
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', -1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', 1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', -nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', @lpSolverReturnSizeWrong)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('lp_solver', @lpSolverReturnTypeWrong)), '');






% max_num_state_evaluations must be an integer >= 0
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', -1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('max_num_state_evaluations', -nan)), '');






% random_seed must be an integer
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('random_seed', -nan)), '');






% timeout must be a number >= 0
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', -1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('timeout', -nan)), '');






% verbose must be an integer [0, 2]
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', -1)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', 3)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', 'a')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', '')), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', [])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {{}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', [1 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', [1; 1])), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {1 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {{1 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {1; 1})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', {{1; 1}})), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', +NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', +nan)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', -NaN)), '');
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', -nan)), '');







% invalid parameter name
assertExceptionThrown(@()solve_qsage_mex(@objFunction, numVars, isingSolver, struct('invalid_parameter_name', 1)), '');

end

% function testSolveQSageMexTBD
% 
% isingSolver = struct;
% isingSolver.solve_ising = @solveIsing;
% isingSolver.qubits = 0 : 127;
% adj = getChimeraAdjacency(4, 4, 4);
% isingSolver.couplers = zeros(2, nnz(adj) / 2);
% index = 1;
% for i = 1 : 128
%     for j = i + 1 : 128
%         if adj(i, j) ~= 0
%             isingSolver.couplers(1, index) = i - 1;
%             isingSolver.couplers(2, index) = j - 1;
%             index = index + 1;
%         end
%     end
% end
% 
% numVars = 20;
% [bestSolution bestEnergy info] = solve_qsage_mex(@objFunction, numVars, isingSolver, struct('verbose', 2));
% 
% end
