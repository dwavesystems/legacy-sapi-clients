% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function solver = sapiSolver(conn, solverName)
%% SAPISOLVER Get a solver handle.
%
%  solver = sapiSolver(conn, solverName)
%
%  Input Parameters
%    conn: a connection handle returned by sapiLocalConnection or
%      sapiRemoteConnection.
%
%    solverName: the name of the requested solver. Must be listed in
%      sapiListSolvers(conn).
%
%  Output
%    solver: a handle to the request solver.
%
%  Solver handles are required by sapiSolve* and sapiAsyncSolve* functions.
%
% Different solvers can take different parameters when used in solving ising/qubo problems.
%
% Hardware solver can take the following parameters:
%
%   annealing_time: a positive integer that sets the duration (in
%                   microseconds) of quantum annealing time.
%                   (must be an integer > 0, default = 1000)
%
%   answer_mode: indicates whether to return a histogram of answers,
%                sorted in order of energy ('histogram'); or to return all
%                answers individually in the order they were read ('raw').
%                (must be 'histogram' or 'raw', default = 'histogram')
%
%   auto_scale: indicates whether h and J values will be rescaled to use as
%               much of the range of h and the range of J as possible, or
%               be used as is. When enabled, h and J values need not lie
%               within the range of h and the range of J (but must still be
%               finite).
%               (must be a boolean value true or false, default = true)
%
%   max_answers: maximum number of answers returned from the solver in
%                histogram mode (which sorts the returned states in order
%                of increasing energy); this is the total number of distinct
%                answers. In raw mode, this limits the returned values to
%                the first max_answers of num_reads samples. Thus, in this
%                mode, max_answers should never be more than num_reads.
%                (must be an integer > 0, default = num_reads)
%
%   num_reads: a positive integer that indicates the number of states
%              (output solutions) to read from the solver in each programming
%              cycle. When a hardware solver is used, a programming cycle
%              programs the solver to the specified h and J values for a
%              given ising problem (or Q values for a given qubo problem).
%              However, since hardware precision is limited, the h and J
%              values (or Q values) realized on the solver will deviate
%              slightly from the requested values. On each programming cycle,
%              the deviation is random.
%              (must be an integer > 0, default = 1)
%
%   programming_thermalization: an integer that gives the time (in microseconds)
%                               to wait after programming the processor in order
%                               for it to cool back to base temperature (i.e.,
%                               post-programming thermalization time). Lower
%                               values will speed up solving at the expense
%                               of solution quality.
%                               (must be an integer > 0, default = 1000)
%
%   readout_thermalization: an integer that gives the time (in microseconds)
%                           to wait after each state is read from the processor
%                           in order for it to cool back to base temperature
%                           (i.e., post-readout thermalization time). Lower
%                           values will speed up solving at the expense of
%                           solution quality.
%                           (must be an integer > 0, deafult = 5)
%
%  The acceptable range and the default value of each field are given in the table below:
%
%    +------------------------------+-------------------------------+-------------------+
%    |            Field             |               Range           |   Default value   |
%    +==============================+===============================+===================+
%    |        annealing_time        |                > 0            |       1000        |
%    +------------------------------+-------------------------------+-------------------+
%    |          answer_mode         |      'histogram' or 'raw'     |    'histogram'    |
%    +------------------------------+-------------------------------+-------------------+
%    |          auto_scale          |          true or false        |        true       |
%    +------------------------------+-------------------------------+-------------------+
%    |         max_answers          |                > 0            |     num_reads     |
%    +------------------------------+-------------------------------+-------------------+
%    |          num_reads           |                > 0            |         1         |
%    +------------------------------+-------------------------------+-------------------+
%    |  programming_thermalization  |                > 0            |       1000        |
%    +------------------------------+-------------------------------+-------------------+
%    |    readout_thermalization    |                > 0            |         5         |
%    +------------------------------+-------------------------------+-------------------+
%
% 'c4-sw_sample' solver can take the following parameters:
%
%   answer_mode: indicates whether to return a histogram of answers, sorted
%                in order of energy ('histogram'); or to return all answers
%                individually in the order they were read ('raw').
%                (must be 'histogram' or 'raw', default = 'histogram')
%
%   beta: Boltzmann distribution parameter. The unnormalized probablity of
%         a sample is proportional to exp(-beta * E) where E is its energy.
%         (must be a number >= 0.0, default = 3.0)
%
%   max_answers: maximum number of answers returned from the solver in
%                histogram mode (which sorts the returned states in order
%                of increasing energy); this is the total number of distinct
%                answers. In raw mode, this limits the returned values to
%                the first max_answers of num_reads samples. Thus, in this
%                mode, max_answers should never be more than num_reads.
%                (must be an integer > 0, default = num_reads)
%
%   num_reads: a positive integer that indicates the number of states
%              (output solutions) to read from the solver in each programming
%              cycle. (must be an integer > 0, default = 1)
%
%   random_seed: random number generator seed. When a value is provided,
%                solving the same problem with the same parameters will
%                produce the same results every time. If no value is provided,
%                a time-based seed is selected.
%                (must be an integer >= 0, default is a time-based seed)
%
%  The acceptable range and the default value of each field are given in the table below:
%
%    +------------------------------+-------------------------------+-------------------+
%    |            Field             |               Range           |   Default value   |
%    +==============================+===============================+===================+
%    |          answer_mode         |      'histogram' or 'raw'     |    'histogram'    |
%    +------------------------------+-------------------------------+-------------------+
%    |             beta             |                > 0.0          |        3.0        |
%    +------------------------------+-------------------------------+-------------------+
%    |         max_answers          |                > 0            |     num_reads     |
%    +------------------------------+-------------------------------+-------------------+
%    |          num_reads           |                > 0            |         1         |
%    +------------------------------+-------------------------------+-------------------+
%    |         random_seed          |                >= 0           |   randomly set    |
%    +------------------------------+-------------------------------+-------------------+
%
% 'c4-sw_optimize' solver can take the following parameters:
%
%   answer_mode: indicates whether to return a histogram of answers, sorted
%                in order of energy ('histogram'); or to return all answers
%                individually in the order they were read ('raw').
%                (must be 'histogram' or 'raw', default = 'histogram')
%
%   max_answers: maximum number of answers returned from the solver in
%                histogram mode (which sorts the returned states in order
%                of increasing energy); this is the total number of distinct
%                answers. In raw mode, this limits the returned values to
%                the first max_answers of num_reads samples. Thus, in this
%                mode, max_answers should never be more than num_reads.
%                (must be an integer > 0, default = num_reads)
%
%   num_reads: a positive integer that indicates the number of states
%              (output solutions) to read from the solver in each programming
%              cycle.
%              (must be an integer > 0, default = 1)
%
%  The acceptable range and the default value of each field are given in the table below:
%
%    +------------------------------+-------------------------------+-------------------+
%    |            Field             |               Range           |   Default value   |
%    +==============================+===============================+===================+
%    |          answer_mode         |     'histogram' or 'raw'      |     'histogram'   |
%    +------------------------------+-------------------------------+-------------------+
%    |         max_answers          |                > 0            |     num_reads     |
%    +------------------------------+-------------------------------+-------------------+
%    |          num_reads           |                > 0            |         1         |
%    +------------------------------+-------------------------------+-------------------+
%
%  See also sapiLocalConnection, sapiRemoteConnection, sapiListSolvers

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if ~ismember(solverName, sapiListSolvers(conn))
  error('No such solver')
end
solver = conn.get_solver(solverName);
end
