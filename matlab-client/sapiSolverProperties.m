% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function properties = sapiSolverProperties(solver)
%% SAPISOLVERPROPERTIES Get solver properties.
%
%  properties = sapiSolverProperties(solver)
%
%  Input Parameters
%    solver: a solver handle.
%
%  Output
%    properties: a structure of solver properties. Contents are
%      solver-specific, see documentation.
%
%  All solvers will have a 'supported_problem_types' parameters whose value
%  is a cell array of problem type strings.
%
%  See also sapiSolver

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

properties = solver.property;
end
