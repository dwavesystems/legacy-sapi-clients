% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function done = sapiAsyncDone(submittedProblem)
%% SAPIASYNCDONE Check if a submitted problem is done.
%
%  done = sapiAsyncDone(submittedProblem)
%
%  Input arguments
%    submittedProblem: submitted problem returned by sapiAsyncSolveIsing or
%      sapiAsyncSolveQubo.
%
%  Output
%    done: logical value indicating whether the problem has been solved.
%
%  Once the problem is done, you can retrieve the answer with the
%  sapiAsyncResult function.
%
%  See also sapiAsyncSolveIsing, sapiAsyncSolveQubo, sapiAsyncResult,
%  sapiCancelSubmittedProblem, sapiAwaitCompletion, sapiAwaitSubmission

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

switch submittedProblem.type
    case 'local'
        done = true;
    case 'remote'
        done = sapiremote_done(submittedProblem.handle);
end
end
