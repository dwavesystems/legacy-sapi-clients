% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testStatus %#ok<STOUT,*DEFNU>
initTestSuite
end

function testStatusSubmitting
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'SUBMITTING';
expectedStatus.last_good_state = 'SUBMITTING';
expectedStatus.remote_status = 'UNKNOWN';
expectedStatus.error_type = 'INTERNAL';
expectedStatus.error_message = '';
expectedStatus.submitted_on = '';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'last_good_state', 'error_type', 'error_message', ...
     'submitted_on', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusSubmitted
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'SUBMITTED';
expectedStatus.last_good_state = 'SUBMITTED';
expectedStatus.remote_status = 'IN_PROGRESS';
expectedStatus.error_type = 'INTERNAL';
expectedStatus.error_message = '';
expectedStatus.submitted_on = 'yesterday';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'last_good_state', 'error_type', 'error_message', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusCompleted
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'DONE';
expectedStatus.last_good_state = 'DONE';
expectedStatus.remote_status = 'COMPLETED';
expectedStatus.error_type = 'INTERNAL';
expectedStatus.error_message = '';
expectedStatus.submitted_on = '1:00';
expectedStatus.solved_on = '2:30';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'last_good_state', 'error_type', 'error_message'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusRetrying
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'RETRYING';
expectedStatus.last_good_state = 'SUBMITTED';
expectedStatus.remote_status = 'PENDING';
expectedStatus.error_type = 'PROTOCOL';
expectedStatus.error_message = 'oops';
expectedStatus.submitted_on = '5:43 am';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, {'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusFailedSolve
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'DONE';
expectedStatus.last_good_state = 'DONE';
expectedStatus.remote_status = 'FAILED';
expectedStatus.error_type = 'SOLVE';
expectedStatus.error_message = 'bad stuff';
expectedStatus.submitted_on = '1:23';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'last_good_state', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusFailedNetwork
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'FAILED';
expectedStatus.last_good_state = 'SUBMITTING';
expectedStatus.remote_status = 'UNKNOWN';
expectedStatus.error_type = 'NETWORK';
expectedStatus.error_message = 'bad stuff';
expectedStatus.submitted_on = '';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'submitted_on', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusFailedAuth
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'FAILED';
expectedStatus.last_good_state = 'SUBMITTING';
expectedStatus.remote_status = 'UNKNOWN';
expectedStatus.error_type = 'AUTH';
expectedStatus.error_message = 'bad stuff';
expectedStatus.submitted_on = '';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'submitted_on', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusFailedMemory
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'FAILED';
expectedStatus.last_good_state = 'SUBMITTING';
expectedStatus.remote_status = 'UNKNOWN';
expectedStatus.error_type = 'MEMORY';
expectedStatus.error_message = 'buy more rams';
expectedStatus.submitted_on = '';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'submitted_on', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end

function testStatusFailedInternal
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));

expectedStatus.problem_id = 'aaa';
expectedStatus.state = 'FAILED';
expectedStatus.last_good_state = 'SUBMITTING';
expectedStatus.remote_status = 'UNKNOWN';
expectedStatus.error_type = 'INTERNAL';
expectedStatus.error_message = '???';
expectedStatus.submitted_on = '';
expectedStatus.solved_on = '';

h = sapiremote_submit(solver, '', [], expectedStatus);
expectedStatus = rmfield(expectedStatus, ...
    {'submitted_on', 'solved_on'});

status = sapiremote_status(h);
assertEqual(status, expectedStatus)
end
