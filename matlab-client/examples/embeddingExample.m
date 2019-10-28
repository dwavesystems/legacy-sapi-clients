% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function embeddingExample

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% define h and j
num_vars = 10;
h = ones(1, num_vars);
j = triu(ones(num_vars), 1);

% define chain strengths to be used
chain_strength = [0.5, 1.0, 2.0];

% use a local solver
conn = sapiLocalConnection;
solver = sapiSolver(conn, 'c4-sw_sample');

size(chain_strength);

A = getHardwareAdjacency(solver);

% find and print embeddings
embeddings = sapiFindEmbedding(j, A, 'verbose', 1);
disp('embeddings:');
embeddings{:}

% main for loop ecapsulating embedded problems of varying chain strengths,
% solving, then unembedding and displaying result
for c = 1:size(chain_strength, 2);
    % embed problem and solve embedded
    [h0, j0, jc] = sapiEmbedProblem(h, j, embeddings, A);
    answer = sapiSolveIsing(solver, h0, j0 + jc * chain_strength(c), 'num_reads', 10);
    newAnswer = sapiUnembedAnswer(answer.solutions, embeddings, 'minimize_energy', h, j);

    % display results for current chain strength value
    fprintf('Results for chain strength = %10.1f\n', chain_strength(c) * -1);
    disp(newAnswer);

    % print unembedded problem result of the form:
    % solution [solution #]
    % var [var #] : [var value] ([qubit index] : [original qubit value] ...)
    for i = 1:size(newAnswer,2)
        fprintf('solution %d:\n', i);
        for j = 1:size(newAnswer,1)
            fprintf('var %d: %d ', j, newAnswer(j, i));
            fprintf('(');
            fprintf('%d:%d, ', [embeddings{j}(:), answer.solutions(embeddings{j}+1, i)]');
            fprintf('\b\b)\n');
        end
    end
end


end
