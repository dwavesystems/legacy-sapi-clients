% Copyright © 2019 D-Wave Systems Inc.
% The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

function test_suite = testSubmittedProblem %#ok<STOUT,*DEFNU>
initTestSuite
end



function testRemoteStatus
sp.type = 'remote';
sp.handle = 'the handle';
status = sapiAsyncStatus(sp);
assertEqual(status, sapiremote_status(sp))
end


function testRemoteStatusTimeReceived
sp.type = 'remote';
sp.handle.test_status.submitted_on = '1:23';
status = sapiAsyncStatus(sp);
assertEqual(status, struct('time_received', '1:23'))

sp.handle.test_status.solved_on = 'later';
status = sapiAsyncStatus(sp);
assertEqual(status, ...
    struct('time_received', '1:23', 'time_solved', 'later'))

sp.handle.test_status = rmfield(sp.handle.test_status, 'submitted_on');
status = sapiAsyncStatus(sp);
assertEqual(status, struct('time_solved', 'later'))
end


function testRemoteRetry
sapiremote_retry('test:reset');
sp.type = 'remote';
sp.handle = 'the handle';
sapiAsyncRetry(sp);
srhist = sapiremote_retry('test:reset');
assertEqual(srhist, {{sp.handle}})
end


function testLocalStatus
sp.type = 'local';
status = sapiAsyncStatus(sp);
assertEqual(status, struct('state', 'DONE'))
end


function testLocalRetry
sapiremote_retry('test:reset');
sp.type = 'local';
sp.handle = 'the handle';
sapiAsyncRetry(sp);
assertTrue(isempty(sapiremote_retry('test:reset')))
end
