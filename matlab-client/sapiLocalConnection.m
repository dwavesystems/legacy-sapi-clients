% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function conn = sapiLocalConnection()
%% SAPILOCALCONNECTION Get a local connection handle.
%
%  conn = sapiLocalConnection
%
%  Output
%    conn: handle to the local connection. Do not rely on its internal
%      structure, it may change in future versions.
%
%  See also sapiRemoteConnection, sapiSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

conn = struct('solver_names', {{'c4-sw_sample' 'c4-sw_optimize', 'ising-heuristic'}}, ...
              'get_solver', @(solverName)getSolver(solverName));

end

function solver = getSolver(solverName)

if strcmp(solverName, 'c4-sw_sample')
    solver = struct('property', createOrangSampleProperty(), ...
                    'solve', @(problemType, problem, solverParams)localOrangSampleSolve(problemType, problem, solverParams), ...
                    'submit', @(problemType, problem, solverParams) submit(problemType, problem, solverParams, @localOrangSampleSolve));
elseif strcmp(solverName, 'c4-sw_optimize')
    solver = struct('property', createOrangOptimizeProperty(), ...
                    'solve', @(problemType, problem, solverParams)localOrangOptimizeSolve(problemType, problem, solverParams), ...
                    'submit', @(problemType, problem, solverParams) submit(problemType, problem, solverParams, @localOrangOptimizeSolve));
elseif strcmp(solverName, 'ising-heuristic')
    solver = struct('property', createOrangHeuristicProperty(), ...
                    'solve', @(problemType, problem, solverParams)localOrangHeuristicSolve(problemType, problem, solverParams), ...
                    'submit', @(problemType, problem, solverParams) submit(problemType, problem, solverParams, @localOrangHeuristicSolve));
else
    error('solver not exist');
end

end

function sp = submit(problemType, problem, params, solveFn)
sp = struct( ...
    'type', 'local', ...
    'solve', @() solveFn(problemType, problem, params));
end

function props = createOrangHeuristicProperty()
props.supported_problem_types = {'ising', 'qubo'};
props.parameters = struct( ...
    'iteration_limit', [], 'max_bit_flip_prob', [], ...
    'max_local_complexity', [], 'min_bit_flip_prob', [], ...
    'local_stuck_limit', [], 'num_perturbed_copies', [], ...
    'num_variables', [], 'random_seed', [], 'time_limit_seconds', []);
end

function props = createOrangSampleProperty()
props.supported_problem_types = {'ising', 'qubo'};
[props.num_qubits, props.qubits, props.couplers, ~] = c4data;
props.parameters = struct( ...
    'num_reads', [], 'max_answers', [], 'answer_mode', [], ...
    'beta', [], 'random_seed', []);
end

function props = createOrangOptimizeProperty()
props.supported_problem_types = {'ising', 'qubo'};
[props.num_qubits, props.qubits, props.couplers, ~] = c4data;
props.parameters = struct( ...
    'num_reads', [], 'max_answers', [], 'answer_mode', [], ...
    'beta', [], 'random_seed', []);
end

function checkParameterName(paramsNames, params, solverName)

paramsFieldNames = fieldnames(params);
for i = 1 : numel(paramsFieldNames)
    if ~any(ismember(paramsNames, paramsFieldNames{i}))
        error('''%s'' is not a valid parameter for ''%s'' solver', paramsFieldNames{i}, solverName);
    end
end

end

function answer = localOrangSampleSolve(problemType, problem, solverParams)

paramsNames = {'num_reads', 'max_answers', 'answer_mode', 'beta', 'random_seed'};
checkParameterName(paramsNames, solverParams, 'c4-sw_sample');

[nq, q, c, vo] = c4data;
oparams = struct('num_vars', {nq}, 'active_vars', {q}, ...
  'active_var_pairs', {c}, 'var_order', {vo});

oparams.num_reads = 1;
if isfield(solverParams, 'num_reads')
    oparams.num_reads = solverParams.num_reads;
end

oparams.max_answers = oparams.num_reads;
if isfield(solverParams, 'max_answers')
    oparams.max_answers = solverParams.max_answers;
end

oparams.answer_histogram = true;
if isfield(solverParams, 'answer_mode')
    if strcmp(solverParams.answer_mode, 'histogram')
        oparams.answer_histogram = true;
    elseif strcmp(solverParams.answer_mode, 'raw')
        oparams.answer_histogram = false;
    else
        error('answer_mode parameter must be ''histogram'' or ''raw''');
    end
end

oparams.beta = 3.0;
if isfield(solverParams, 'beta')
    oparams.beta = solverParams.beta;
end

if isfield(solverParams, 'random_seed')
    oparams.random_seed = solverParams.random_seed;
end

answer = sapilocal_orangSample(problemType, problem, oparams);

end

function answer = localOrangOptimizeSolve(problemType, problem, solverParams)

paramsNames = {'num_reads', 'max_answers', 'answer_mode'};
checkParameterName(paramsNames, solverParams, 'c4-sw_optimize');

[nq, q, c, vo] = c4data;
oparams = struct('num_vars', {nq}, 'active_vars', {q}, ...
  'active_var_pairs', {c}, 'var_order', {vo});

oparams.num_reads = 1;
if isfield(solverParams, 'num_reads')
    oparams.num_reads = solverParams.num_reads;
end

oparams.max_answers = oparams.num_reads;
if isfield(solverParams, 'max_answers')
    oparams.max_answers = solverParams.max_answers;
end

oparams.answer_histogram = true;
if isfield(solverParams, 'answer_mode')
    if strcmp(solverParams.answer_mode, 'histogram')
        oparams.answer_histogram = true;
    elseif strcmp(solverParams.answer_mode, 'raw')
        oparams.answer_histogram = false;
    else
        error('answer_mode parameter must be ''histogram'' or ''raw''');
    end
end

answer = sapilocal_orangOptimize(problemType, problem, oparams);

end

function answer = localOrangHeuristicSolve(problemType, problem, solverParams)

paramsNames = {'iteration_limit', 'max_bit_flip_prob', 'max_local_complexity', 'min_bit_flip_prob', 'local_stuck_limit', ...
               'num_perturbed_copies', 'num_variables', 'random_seed', 'time_limit_seconds'};
checkParameterName(paramsNames, solverParams, 'ising-heuristic');

orangHeuristicSolverParams = struct;

orangHeuristicSolverParams.iteration_limit = 10;
if isfield(solverParams, 'iteration_limit')
    orangHeuristicSolverParams.iteration_limit = solverParams.iteration_limit;
end

orangHeuristicSolverParams.max_bit_flip_prob = 1.0 / 8.0;
if isfield(solverParams, 'max_bit_flip_prob')
    orangHeuristicSolverParams.max_bit_flip_prob = solverParams.max_bit_flip_prob;
end

orangHeuristicSolverParams.max_local_complexity = 9;
if isfield(solverParams, 'max_local_complexity')
    orangHeuristicSolverParams.max_local_complexity = solverParams.max_local_complexity;
end

orangHeuristicSolverParams.min_bit_flip_prob = 1.0 / 32.0;
if isfield(solverParams, 'min_bit_flip_prob')
    orangHeuristicSolverParams.min_bit_flip_prob = solverParams.min_bit_flip_prob;
end

orangHeuristicSolverParams.local_stuck_limit = 8;
if isfield(solverParams, 'local_stuck_limit')
    orangHeuristicSolverParams.local_stuck_limit = solverParams.local_stuck_limit;
end

orangHeuristicSolverParams.num_perturbed_copies = 4;
if isfield(solverParams, 'num_perturbed_copies')
    orangHeuristicSolverParams.num_perturbed_copies = solverParams.num_perturbed_copies;
end

orangHeuristicSolverParams.num_variables = 0;
if isfield(solverParams, 'num_variables')
    orangHeuristicSolverParams.num_variables = solverParams.num_variables;
end

if isfield(solverParams, 'random_seed')
    orangHeuristicSolverParams.random_seed = solverParams.random_seed;
end

orangHeuristicSolverParams.time_limit_seconds = 5.0;
if isfield(solverParams, 'time_limit_seconds')
    orangHeuristicSolverParams.time_limit_seconds = solverParams.time_limit_seconds;
end

answer = sapilocal_orangHeuristic(problemType, problem, orangHeuristicSolverParams);

end

function [num_qubits, qubits, couplers, var_order] = c4data

persistent num_qubits0 qubits0 couplers0 var_order0

if isempty(num_qubits0)
    num_qubits0 = 128;
    qubits0 = (0:127)';
    [cr cc] = find(triu(getChimeraAdjacency(4, 4, 4), 1));
    couplers0 = [cr-1 cc-1]';
    var_order0 = [ ...
          0  32  64  96   1  33  65  97   2  34  66  98   3  35  67  99 ...
          8  40  72 104   9  41  73 105  10  42  74 106  11  43  75 107 ...
         16  48  80 112  17  49  81 113  18  50  82 114  19  51  83 115 ...
         24  56  88 120  25  57  89 121  26  58  90 122  27  59  91 123 ...
          4   5   6   7  36  37  38  39  68  69  70  71 100 101 102 103 ...
         12  13  14  15  44  45  46  47  76  77  78  79 108 109 110 111 ...
         20  21  22  23  52  53  54  55  84  85  86  87 116 117 118 119 ...
         28  29  30  31  60  61  62  63  92  93  94  95 124 125 126 127]';
end

num_qubits = num_qubits0;
qubits = qubits0;
couplers = couplers0;
var_order = var_order0;

end
