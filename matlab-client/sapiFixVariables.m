% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function [fixedVariables, new_Q, offset] = sapiFixVariables(Q, method)

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if nargin < 2
    method = 'optimized';
end

[fixedVariables, new_Q, offset] = fix_variables_mex(Q, method);

end
