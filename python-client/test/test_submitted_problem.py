# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from flexmock import flexmock
from dwave_sapi2.local import _LocalSubmittedProblem
from dwave_sapi2.remote import _RemoteSubmittedProblem


def test_remote_retry():
    rsp = flexmock()
    rsp.should_receive('retry')
    sp = _RemoteSubmittedProblem(rsp)
    sp.retry()


def test_remote_status():
    rsp = flexmock()
    rsp.should_receive('status').and_return('the status')
    sp = _RemoteSubmittedProblem(rsp)
    assert sp.status() == 'the status'


def test_remote_status_times():
    rsp = flexmock()
    (rsp.should_receive('status')
        .and_return({'submitted_on': '1:23'})
        .and_return({'solved_on': '4:56'})
        .and_return({'submitted_on': 'earlier', 'solved_on': 'now'}))
    sp = _RemoteSubmittedProblem(rsp)

    assert sp.status() == {'time_received': '1:23'}
    assert sp.status() == {'time_solved': '4:56'}
    assert sp.status() == {'time_received': 'earlier', 'time_solved': 'now'}


def test_local_retry():
    sp = _LocalSubmittedProblem(None, None, None, None)
    sp.retry()


def test_local_status():
    sp = _LocalSubmittedProblem(None, None, None, None)
    assert sp.status() == {'state': 'DONE'}
