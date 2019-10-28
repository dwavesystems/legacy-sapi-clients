% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function answer = sapiAsyncResult(submittedProblem)
%% SAPIASYNCRESULT Retrieve the answer from a sumbitted problem.
%
%  answer = sapiAsyncResult(submittedProblem)
%
%  Input arguments
%    submittedProblem: submitted problem returned by sapiAsyncSolveIsing or
%      sapiAsyncSolveQubo.
%
%  Output
%    answer: the answer to the problem. The format will be identical to
%      answers returned by the synchronous solving functions sapiSolveIsing
%      and sapiSolveQubo.
%
%  Attempting to retrieve the answer to a problem that is not done will
%  trigger an error. Use sapiAsyncDone to check whether or not the problem
%  is done.
%
%  See also sapiAsyncSolveIsing, sapiAsyncSolveQubo, sapiAsyncDone,
%  sapiCancelSubmittedProblem, sapiAwaitCompletion, sapiAwaitSubmission

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

switch submittedProblem.type
    case 'local'
        answer = submittedProblem.solve;
    case 'remote'
        ranswer = sapiremote_answer(submittedProblem.handle);
        if strcmp(ranswer{1}, 'ising') || strcmp(ranswer{1}, 'qubo')
            answer = sapiremote_decodeqp(ranswer{1}, ranswer{2});
        end
end
end
