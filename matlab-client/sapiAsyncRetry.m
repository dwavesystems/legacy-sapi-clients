% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function sapiAsyncRetry(submittedProblem)
%% SAPIASYNCRETRY Retry a failed submitted problem.
%
%  sapiAsyncRetry(submittedProblem)
%
%  Input arguments
%    submittedProblem: submitted problem returned by sapiAsyncSolveIsing or
%      sapiAsyncSolveQubo.
%
%  This function is only effective for remote problems that have failed for
%  non-solving reasons (e.g. network problems).

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

if strcmp(submittedProblem.type, 'remote')
    sapiremote_retry(submittedProblem.handle);
end
end
