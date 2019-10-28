% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

%sapilocal_orangHeuristic heuristic solver can handle problems of arbitrary structure.
%   result = sapilocal_orangHeuristic(problemType, problem, params)
%
%   problemType: 'ising' or 'qubo'.
%   problem: problem matrix.  For QUBO problems, this is the matrix Q for the
%     energy function is x'*Q*x.  For Ising problem this is the matrix
%     J+diag(h) for the energy function h'*s+s'*J*s.  Zeros are ignored and
%     the matrix need not be square.
%   params: a structure with the following fields (required unless noted):
%     iteration_limit: Maximum number of solver iterations. This does not
%       include the initial local search.
%     min_bit_flip_prob, max_bit_flip_prob: Bit flip probability range. The
%       probability of flipping each bit is constant for each perturbed
%       solution copy but varies across copies. The probabilities used are
%       linearly interpolated between min_bit_flip_prob and max_bit_flip_prob.
%       Larger values allow more exploration of the solution space and easier
%       escapes from local minima but may also discard nearly-optimal solutions.
%     max_local_complexity: Maximum complexity of subgraphs used during local
%       search. The run time and memory requirements of each step in the local
%       search are exponential in this parameter. Larger values allow larger
%       subgraphs (which can improve solution quality) but require much more
%       time and space.
%       Subgraph 'complexity' here means treewidth + 1.
%     local_stuck_limit: Number of consecutive local search steps that do not
%       improve solution quality to allow before determining a solution to be
%       a local optimum. Larger values produce more thorough local searches but
%       increase run time.
%     num_perturbed_copies: Number of perturbed solution copies created at each
%       iteration. Run time is linear in this value.
%     num_variables: Lower bound on the number of variables. This solver can
%       accept problems of arbitrary structure and the size of the solution
%       returned is determined by the maximum variable index in the problem.
%       The size of the solution can be increased by setting this parameter.
%     random_seed (optional): Random number generator seed. When a value is
%       provided, solving the same problem with the same parameters will produce
%       the same results every time. If no value is provided, a time-based seed
%       is selected.
%       The use of a wall clock-based timeout may in fact cause different results
%       with the same random_seed value. If the same problem is run under different
%       CPU load conditions (or on computers with different perfomance), the amount
%       of work completed may vary despite the fact that the algorithm is deterministic.
%       If repeatability of results is important, rely on the iteration_limit parameter
%       rather than the time_limit_seconds parameter to set the stopping criterion.
%     time_limit_seconds: Maximum wall clock time in seconds. Actual run times will
%       exceed this value slightly.
%
%   result: a structure with fields:
%     energies: solution energy.
%     solutions: Unused variables will have a value of 3.  Used variables will be
%       elements of {-1, +1} for Ising problems or {0, 1} for QUBO problems.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
