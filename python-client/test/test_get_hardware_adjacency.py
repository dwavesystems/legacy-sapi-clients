# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

from flexmock import flexmock
from dwave_sapi2.util import get_hardware_adjacency


def test():
    solver = flexmock(properties={'couplers': [(0, 1), (3, 2), [10, 100]]})
    assert get_hardware_adjacency(solver) == set(
        [(0, 1), (1, 0), (2, 3), (3, 2), (10, 100), (100, 10)])