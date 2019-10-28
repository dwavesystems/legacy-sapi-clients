% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function testRetry
solvers = sapiremote_solvers(sapiremote_connection('', ''));
solver = solvers(arrayfun(@(s) strcmp(s.id, 'status'), solvers));
initStatus.problem_id = 'aaa';
initStatus.state = 'FAILED';
initStatus.last_good_state = 'SUBMITTING';
initStatus.remote_status = 'UNKNOWN';
initStatus.error_type = 'NETWORK';
initStatus.error_message = 'bad stuff';
initStatus.submitted_on = '123';
initStatus.solved_on = '456';

h = sapiremote_submit(solver, '', [], initStatus);
sapiremote_retry(h)
expectedStatus = initStatus;
expectedStatus.state = 'RETRYING';
assertEqual(sapiremote_status(h), expectedStatus)
end
