% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function answer = sapiremote_solve(solver, type, problem, params)

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

h = sapiremote_submit(solver, type, problem, params);
if ~sapiremote_awaitcompletion({h}, 1, Inf)
    error('problem not done');
end
answer = sapiremote_answer(h);
end
