% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function sps = sapiAwaitSubmission(sps, timeout)
%% SAPIAWAITSUBMISSION waits for async problems to be submitted.
%
%  newSubmittedProblems = sapiAwaitSubmission(submittedProblems, timeout)
%
%  Input parameters
%    submittedProblems: cell array of submitted problems
%      (returned by sapiAsyncSolveIsing/sapiAsyncSolveQubo)
%
%    timeout: maximum time to wait (in seconds)
%
%  Output
%    newSubmittedProblems: cell array of saveable asychronous problem
%      handles or empty matrices for corresponding problems that aren't
%      submitted before timeout
%
%  The function allows users to save asynchronous problems and restore
%  them possibly in a new MATLAB instance.
%
%  See also sapiAsyncSolveIsing, sapiAsyncSolveQubo, sapiAsyncDone,
%  sapiAsyncResult, sapiCancelSubmittedProblem, sapiAwaitCompletion,
%  sapiAwaitCompletion

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

remote = cellfun(@(sp) strcmp(sp.type, 'remote'), sps);
phs = cellfun(@(sp) sp.handle, sps(remote), 'UniformOutput', false);
phs = sapiremote_awaitsubmission(phs, timeout);
for ii=1:numel(phs)
    if ~isempty(phs{ii})
        phs{ii} = struct('type', 'remote', 'handle', phs{ii});
    end
end
sps(remote) = phs;
end
