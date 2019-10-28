% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%FIND_EMBEDDING_MEX Find am embedding of a QUBO/Ising in a graph.
%
%  embeddings = find_embedding_mex(Q, A, params)
%  (can be interrupted by Ctrl-C, will return the best embedding found so far.)
%
%  Attempts to find an embedding of a QUBO in a graph. This function
%  is entirely heuristic: failure to return an embedding does not
%  prove that no embedding exists.
%
%  Input parameters:
%
%    Q: Edge structures of a problem, can be Qubo/Ising. The embedder only cares
%       about the edge structure (i.e. which variables have a nontrivial
%       interactions), not the coefficient values.
%
%    A: Adjacency matrix of the graph, as returned by
%       getChimeraAdjacency() or getHardwareAdjacency().
%
%    params: structure of parameters. Must be a structure.
%
%  Output:
%
%    embeddings: A cell array of embeddings, embeddings{i} is the set of qubits
%                representing logical variable i. The index is 0-based.
%                This embeddings return value can be used in sapiEmbeddingSolver.
%                If the algorithm fails, the output is an empty cell array.
%
%   parameters for find_embedding_mex:
%
%   fast_embedding: true/false, tries to get an embedding quickly, without worrying about
%                   chain length.
%                   (must be a boolean, default = false)
%
%   max_no_improvement: number of rounds of the algorithm to try from the current
%                       solution with no improvement. Each round consists of an attempt to find an
%                       embedding for each variable of Q such that it is adjacent to all its
%                       neighbours.
%                       (must be an integer >= 0, default = 10)
%
%   random_seed: seed for random number generator that find_embedding_mex uses
%                (must be an integer >= 0, default is randomly set)
%
%   timeout: Algorithm gives up after timeout seconds.
%            (must be a number >= 0, default is approximately 1000 seconds)
%
%   tries: The algorithm stops after this number of restart attempts. On Vesuvius,
%          each restart takes between 1 and 60 seconds typically.
%          (must be an integer >= 0, default = 10)
%
%   verbose: 0/1.
%            (must be an integer [0, 1], default = 0)
%            when verbose is 1, the output information will be like:
%            try ...
%            max overfill = ..., num max overfills = ...
%            Embedding found. Minimizing chains...
%            max chain size = ..., num max chains = ..., qubits used = ...
%            detailed explanation of the output information:
%              try: ith (0-based) try
%              max overfill: largest number of variables represented in a qubit
%              num max overfills: the number of qubits that has max overfill
%              max chain size: largest number of qubits representing a single variable
%              num max chains: the number of variables that has max chain size
%              qubits used: the total number of qubits used to represent variables

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
