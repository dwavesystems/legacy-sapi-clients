% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function done = sapiAwaitCompletion(sps, minDone, timeout)
%% SAPIAWAITCOMPLETION waits for problems to complete.
%
%  done = sapiAwaitCompletion(submittedProblems, minDone, timeout)
%
%  Input parameters
%    submittedProblems: cell array of asynchronous problems
%      (returned by sapiAsyncSolveIsing/sapiAsyncSolveQubo)
%
%    minDone: minimum number of problems that must be completed before
%      returning (without timeout).
%
%    timeout: maximum time to wait (in seconds)
%
%  Output
%    done: true if returning because enough problems completed, false if
%      returning because of timeout
%
%  See also sapiAsyncSolveIsing, sapiAsyncSolveQubo, sapiAsyncResult,
%  sapiCancelSubmittedProblem, sapiAwaitCompletion, sapiAwaitSubmission

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

remote = cellfun(@(sp) strcmp(sp.type, 'remote'), sps);
minDone = minDone - sum(~remote);
phs = cellfun(@(sp) sp.handle, sps(remote), 'UniformOutput', false);
done = sum(sapiremote_awaitcompletion(phs, minDone, timeout)) >= minDone;
end
