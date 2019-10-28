# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

import dwave_sapi2.core
import dwave_sapi2.local
import dwave_sapi2.remote
import time
import pytest
from flexmock import flexmock


def test_no_problems(monkeypatch):
    def dummy_await_completion(*_):
        return True

    monkeypatch.setattr('sapiremote.await_completion', dummy_await_completion)

    start = time.time()
    r = dwave_sapi2.core.await_completion([], 1, 2)
    end = time.time()
    assert r
    assert end - start < 0.5


def test_remote_done(monkeypatch):
    def dummy_await_completion(*_):
        return True

    monkeypatch.setattr('sapiremote.await_completion', dummy_await_completion)

    rp = flexmock()
    rp.should_receive('done').and_return(True)
    sp = [dwave_sapi2.remote._RemoteSubmittedProblem(rp) for _ in range(15)]

    start = time.time()
    r = dwave_sapi2.core.await_completion(sp, len(sp), 2)
    end = time.time()
    assert r
    assert end - start < 0.5


def test_remote_not_done(monkeypatch):
    def dummy_await_completion(_, __, timeout):
        time.sleep(timeout)
        return False

    monkeypatch.setattr('sapiremote.await_completion', dummy_await_completion)

    rp = flexmock()
    rp.should_receive('done').and_return(False)
    sp = [dwave_sapi2.remote._RemoteSubmittedProblem(rp) for _ in range(15)]

    start = time.time()
    r = dwave_sapi2.core.await_completion(sp, len(sp), 0.1)
    end = time.time()
    assert not r
    assert end - start >= 0.1


@pytest.mark.parametrize('timeout', [
    0, float('inf'), float('-inf'), -1, 1e100, -1e100
])
def test_weird_timeouts(monkeypatch, timeout):
    def dummy_await_completion(*_):
        return True

    monkeypatch.setattr('sapiremote.await_completion', dummy_await_completion)

    rp = flexmock()
    rp.should_receive('done').and_return(True)
    sp = [dwave_sapi2.remote._RemoteSubmittedProblem(rp) for _ in range(20)]

    start = time.time()
    dwave_sapi2.core.await_completion(sp, len(sp), timeout)
    end = time.time()
    assert end - start < 0.5


def test_local():
    sp = dwave_sapi2.local._LocalSubmittedProblem(None, None, None, None)
    start = time.time()
    assert dwave_sapi2.core.await_completion([sp], 1, 10)
    end = time.time()
    assert end - start < 1


def test_bad_submittedproblem_type():
    with pytest.raises(TypeError):
        dwave_sapi2.core.await_completion(['not a submitted problem'], 1, 1)
