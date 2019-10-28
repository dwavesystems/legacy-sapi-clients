% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testFindEmbeddingMex %#ok<STOUT,*DEFNU>
initTestSuite
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
tCol = spones(diag(ones(1, n - 1), 1) + diag(ones(1, n - 1), -1));                % [|j-j'|=1]
A = kron(speye(m * n), kron(spones([0 1; 1 0]), ones(k))) + ...             % bipartite connections
   kron(kron(tRow, speye(n)), kron(spones([1 0; 0 0]), speye(k))) +  ...  % connections between rows
   kron(kron(speye(m), tCol), kron(spones([0 0; 0 1]), speye(k)));        % connections between cols

end

function testFindEmbeddingMexInvalidParameters

QSize = 20;
Q = ones(QSize) - eye(QSize);

M = 5;
N = 5;
L = 4;
A = getChimeraAdjacency(M, N, L);

% fast_embedding must be a boolean
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', 1)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('fast_embedding', -nan)), '');






% max_no_improvement must be an integer >= 0
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', -1)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('max_no_improvement', -nan)), '');
% matlab static_cast<T>(*mxGetPr(a)) will cast inf to be INT_MIN !!!
%find_embedding_mex(Q, A, struct('max_no_improvement', inf));






% random_seed must be an integer
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('random_seed', -nan)), '');






% timeout must be a number >= 0
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', -1)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('timeout', -nan)), '');






% max_no_improvement must be an integer >= 0
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', -1)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('tries', -nan)), '');






% verbose must be an integer [0, 1]
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', -1)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', 2)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', 'a')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', '')), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', [])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {{}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', [1 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', [1; 1])), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {1 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {{1 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {1; 1})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', {{1; 1}})), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', +NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', +nan)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', -NaN)), '');
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('verbose', -nan)), '');






% invalid parameter name
assertExceptionThrown(@()find_embedding_mex(Q, A, struct('invalid_parameter_name', 1)), '');




end

function testFindEmbeddingMexEmptyQ

Q = [];

M = 5;
N = 5;
L = 4;
A = getChimeraAdjacency(M, N, L);

embeddings = find_embedding_mex(Q, A, struct());
assertEqual(length(embeddings), 0);

end

function testFindEmbeddingMexEmptyA

QSize = 20;
Q = ones(QSize) - eye(QSize);

A = [];

embeddings = find_embedding_mex(Q, A, struct());
assertEqual(length(embeddings), 0);

end

function testFindEmbeddingMexEmptyQandA

embeddings = find_embedding_mex([], [], struct());
assertEqual(length(embeddings), 0);

end

function testFindEmbeddingMexNoEmbeddingSolution

Q = [0,1,1;0,0,1;0,0,0];
A = [0,1,0;0,0,1;0,0,0];

embeddings = find_embedding_mex(Q, A, struct());
assertEqual(length(embeddings), 0);

end
