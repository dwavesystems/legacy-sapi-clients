% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function sapiCancelSubmittedProblem(submittedProblem)
%% SAPICANCELSUBMITTEDPROBLEM Cancel a submitted problem.
%
%  sapiCancelSubmittedProblem(submittedProblem)
%
%  Input arguments
%    submittedProblem: submitted problem handle returned by sapiAsyncSolveIsing or
%      sapiAsyncSolveQubo.
%
%  See also sapiAsyncSolveIsing, sapiAsyncSolveQubo, sapiAsyncDone, sapiAsyncResult

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if strcmp(submittedProblem.type, 'remote')
    sapiremote_cancel(submittedProblem.handle)
end
end
