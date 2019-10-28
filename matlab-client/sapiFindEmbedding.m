% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function embeddings = sapiFindEmbedding(S, A, varargin)
%% SAPIFINDEMBEDDING Find am embedding of an ising/qubo in a graph.
%
%  embeddings = sapiFindEmbedding(S, A)
%  embeddings = sapiFindEmbedding(S, A, params)
%  embeddings = sapiFindEmbedding(S, A, 'paramName', value, ...)
%  (can be interrupted by Ctrl-C, will return the best embedding found so far.)
%
%  Attempts to find an embedding of a qubo/ising in a graph. This function
%  is entirely heuristic: failure to return an embedding does not
%  prove that no embedding exists.
%
%  Input parameters:
%
%    S: Edge structures of a problem, can be qubo/ising. The embedder only cares
%       about the edge structure (i.e. which variables have nontrivial
%       interactions), not the coefficient values.
%
%    A: Adjacency matrix of the graph, as returned by
%       getChimeraAdjacency() or getHardwareAdjacency().
%
%    params: structure of parameters.
%            'paramName', value: individual parameters.
%
%  Parameters may be given as arbitrary combinations of params
%  structures and 'paramName'/value pairs.  Any 'paramName'/value pairs
%  must not be split (i.e. 'paramName' and value must appear
%  consecutively). Duplicate parameter names are allowed but only the
%  last value will be used.
%
%   parameters for sapiFindEmbedding:
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
%   random_seed: seed for random number generator that sapiFindEmbedding uses
%                (must be an integer >= 0, default is randomly set)
%
%   timeout: Algorithm gives up after timeout seconds.
%            (must be a number >= 0, default is approximately 1000.0 seconds)
%
%   tries: The algorithm stops after this number of restart attempts.
%          (must be an integer >= 0, default = 10)
%
%   verbose: control the output information.
%            (must be an integer [0 1], default = 0)
%            when verbose is 1, the output information will be like:
%            component ..., try ...:
%            max overfill = ..., num max overfills = ...
%            Embedding found. Minimizing chains...
%            max chain size = ..., num max chains = ..., qubits used = ...
%            detailed explanation of the output information:
%              component: process ith (0-based) component, the algorithm tries to embed
%                         larger strongly connected components first, then smaller components
%              try: jth (0-based) try
%              max overfill: largest number of variables represented in a qubit
%              num max overfills: the number of qubits that has max overfill
%              max chain size: largest number of qubits representing a single variable
%              num max chains: the number of variables that has max chain size
%              qubits used: the total number of qubits used to represent variables
%
% The acceptable range and the default value of each field are given in the table below:
%
%    +-----------------------+-----------------+-------------------+
%    |          Field        |      Range      |   Default value   |
%    +=======================+=================+===================+
%    |    fast_embedding     |  true or false  |       false       |
%    +-----------------------+-----------------+-------------------+
%    |  max_no_improvement   |       >= 0      |        10         |
%    +-----------------------+-----------------+-------------------+
%    |      random_seed      |      >= 0       |    randomly set   |
%    +-----------------------+-----------------+-------------------+
%    |        timeout        |      >= 0.0     |      1000.0       |
%    +-----------------------+-----------------+-------------------+
%    |         tries         |      >= 0       |        10         |
%    +-----------------------+-----------------+-------------------+
%    |        verbose        |      [0 1]      |         0         |
%    +-----------------------+-----------------+-------------------+
%
%  Output:
%
%    embeddings: A cell array of embeddings, embeddings{i} is the set of qubits
%                representing logical variable i. The index is 0-based.
%                This embeddings return value can be used in sapiEmbeddingSolver.
%                If the algorithm fails, the output is an empty cell array.
%
%  See also sapiEmbeddingSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

params = sapi_solveParams(varargin);
embeddings = find_embedding_mex(S, A, params);

end
