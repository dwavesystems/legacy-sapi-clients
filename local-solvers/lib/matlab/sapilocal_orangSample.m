% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%sapilocal_orangSample Boltzmann sampling solver.
%   result = sapilocal_orangSample(problemType, problem, params)
%
%   problemType: 'ising' or 'qubo'.
%   problem: problem matrix.  For QUBO problems, this is the matrix Q for the
%     energy function is x'*Q*x.  For Ising problem this is the matrix
%     J+diag(h) for the energy function h'*s+s'*J*s.  Zeros are ignored and
%     the matrix need not be square.
%   params: a structure with the following fields (required unless noted):
%     num_vars: number of variables in the solution.
%     active_vars: vector of variable indices that may be used in problems.
%       Values are zero-based and therefore not equal to problem matrix
%       indices.
%     active_var_pairs: 2-by-N matrix of allowed variables in quadratic
%       terms.  Each column is a pair.  All entries must also appear in the
%       active_vars field.
%     var_order: variable elimination order.  Must be some permutation of the
%       unique entries in active_vars.
%     num_reads: number of samples to draw.
%     max_answers: maximum number of solutions to return.  In histogram mode,
%       this is the number of distinct solutions.  Otherwise, it behaves the
%       same as num_reads and the lesser value is used.
%     answer_histogram: logical value indicating whether or not to return
%       solutions as a histogram of distinct solutions.  If false, duplicate
%       solutions may be present.
%     beta: Boltzmann distribution parameter.  The unnormalized probablity
%       of a sample is proportional to exp(-beta*E) where E is its energy.
%     random_seed (optional): seed value for the random number generator.
%       The function will return identical results for identical problems and
%       parameters when this value is provided.  If missing or empty, a
%       time-based value is used.
%
%   result: a structure with fields:
%     energies: row vector of sample energies.  When the answer_histogram
%       parameter is true, this vector will be sorted in ascending order.
%     solutions: matrix of samples.  Each column is a single sample and its
%       associated energy and count appear in the same columns of the
%       energies and num_occurrences fields, respectively.  Unused variables
%       will have a value of 3.  Used variables will be elements of {-1, +1}
%       for Ising problems or {0, 1} for QUBO problems.
%     num_occurrences: number of occurrences of each sample.  Not present
%       unless answer_histogram is true (or in the trivial case when zero
%       samples are requested).

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
