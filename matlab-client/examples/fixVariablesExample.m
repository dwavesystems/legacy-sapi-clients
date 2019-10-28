% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function fixVariablesExample

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

% define Q
Q = [1 0 -2; 0 -1 0; 0 0 1];

% Fix variables - standard
[fixedVariables, new_Q, offset] = sapiFixVariables(Q, 'standard');

% Display standard method results
disp('Standard method:');
disp('Fixed variables:');
disp(fixedVariables);

disp('Q values for new problem:');
disp(new_Q);

disp('Offset:');
disp(offset);

% Fix variables - optimized
[fixedVariables, new_Q, offset] = sapiFixVariables(Q, 'optimized');

% Display optimized method results
disp('Optimized method:');
disp('Fixed variables:');
disp(fixedVariables);

disp('Q values for new problem:');
disp(new_Q);

disp('Offset:');
disp(offset);

end
