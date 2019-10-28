% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

% FIX_VARIABLES_MEX  Fix variables for QUBO problems
%
%  [fixedVars newQ offset] = fix_variables_mex(Q);
%  [fixedVars newQ offset] = fix_variables_mex(Q, 'optimized');
%  [fixedVars newQ offset] = fix_variables_mex(Q, 'standard');
%
%  Input parameters:
%  Q: must be a full or sparse square NxN matrix defining a QUBO problem
%  x'*Q*x [**required**]
%
%  method: If not set, it will uses its default value 'optimized'
%          If it is set, must be a string of 'optimized' or 'standard'
%            'optimized': roof-duality & strongly connected components (default value)
%            'standard': roof-duality
%          [**optional**]
%
%  fix_variables_mex uses maximum flow in the implication network to
%  correctly fix variables (that is, one can find an assignment for the
%  other variables that attains the optimal value). The variables that
%  roof duality fixes will take the same values in all optimal solutions.
%
%  Using strongly connected components can fix more variables, but in some
%  optimal solutions these variables may take different values.
%
%  In summary:
%    * All the variables fixed by method = 'standard' will also be fixed by method = 'optimized' (reverse is not true)
%    * All the variables fixed by method = 'standard' will take the same value in every optimal solution
%    * There exists at least one optimal solution that has the fixed values as given by method = 'optimized'
%
%  Thus, method = 'standard' is a subset of method = 'optimized' as any variable that is fixed by method = 'standard'
%  will also be fixed by method = 'optimized' and additionally, method = 'optimized' may fix some variables that
%  method = 'standard' could not. For this reason, method = 'optimized' takes longer than method = 'standard'.
%
%  Return values:
%    fixedVars: variables that can be fixed with value 1 or 0
%    newQ: new Q of unfixed variables
%    offset: x' * newQ * x + offset = x' * Q * x

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
