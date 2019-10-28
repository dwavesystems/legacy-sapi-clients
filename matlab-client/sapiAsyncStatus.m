% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function status = sapiAsyncStatus(submittedProblem)

%% SAPIASYNCSTATUS Retrieve status of a submitted problem.
%
%  status = sapiAsyncStatus(submittedProblem)
%
%  Input arguments
%    submittedProblem: submitted problem returned by sapiAsyncSolveIsing or
%      sapiAsyncSolveQubo.
%
%  Output
%    status: structure containing problem status, described below.
%
%  Status Structure Fields
%
%    state: describes the state of the problem as seen by the client.
%       This field is always present.  Possible values are:
%       'SUBMITTING': the problem is in the process of being submitted.
%       'SUBMITTED': the problem has been submitted but is not done yet.
%       'DONE': the problem is done, meaning either that it was
%         successfully completed or that solving failed.
%       'FAILED': an error occurred while determining the actual state.
%         This condition does not imply anything about whether the problem
%         itself has succeeded or not.
%       'RETRYING': similar to 'FAILED' but the client is actively trying
%         to fix the problem.
%
%    remote_status: describes the state of the problem as reported by the
%      server.  This field is present only for remote problems.  Possible
%      values are:
%      'PENDING': the problem is waiting in a queue.
%      'IN_PROGRESS': processing has started for the problem.
%      'COMPLETED': the problem has completed successfully.
%      'FAILED': an error occurred while solving the problem.
%      'CANCELED': the problem was cancelled by the user.
%      'UNKNOWN': the client has not yet received information from the
%        server (i.e. state has not reached 'SUBMITTED').
%
%    problem_id: the remote problem ID for this problem.  This field is
%      only present for problems submitted to remote solvers.  It is an
%      empty string until the 'SUBMITTED' state is reached.
%
%    last_good_state: last good value of the state field.  The
%      'SUBMITTING', 'SUBMITTED', and 'DONE' states are "good," while
%      'FAILED', and 'RETRYING' are "bad."  This field is present only when
%      the state field is bad and its value is the last good value of the
%      state field.
%
%    error_type: machine-readable error category.  This field is present 
%      only when either:
%        * state is 'FAILED', or
%        * state is 'RETRYING', or
%        * state is 'DONE' and remote_status is not 'COMPLETED'.
%      Possible values are:
%      'NETWORK': network communication failed.
%      'PROTOCOL': client couldn't understand a response from the server.
%        Possible causing include communication errors between intermediate
%        servers, client or server bugs.
%      'AUTH': authentication failed.
%      'SOLVE': solving failed.
%      'MEMORY': out of memory.
%      'INTERNAL': catch-all value for unexpected errors.
%
%    error_message: human-readable error message.  Present exectly when
%      error_type is.
%
%    time_received: time at which the server received the problem.
%      Present once the problem has been received.
%
%    time_solved: time at which the problem was solved.  Present once
%      the problem has been solved.

% Proprietary Information D-Wave Systems Inc.
% Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
% Notice this code is licensed to authorized users only under the
% applicable license agreement see eula.txt
% D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

switch submittedProblem.type
    case 'local'
        status = struct('state', 'DONE');
    case 'remote'
        status = sapiremote_status(submittedProblem.handle);
        if isfield(status, 'submitted_on')
            status.time_received = status.submitted_on;
            status = rmfield(status, 'submitted_on');
        end
        if isfield(status, 'solved_on')
            status.time_solved = status.solved_on;
            status = rmfield(status, 'solved_on');
        end
end
end
