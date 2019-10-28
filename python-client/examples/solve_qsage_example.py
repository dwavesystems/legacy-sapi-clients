# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.local import local_connection
from dwave_sapi2.qsage import solve_qsage
import sys


# This example shows how to use sapi_solveQSage function to heuristically minimize arbitrary objective functions.
#
# usage:
#  solve_qsage_example


# objective function can be a class or a function

class ObjClass:
    def __call__(self, states):

        num_states = len(states)
        state_len = len(states[0])
        ret = []
        for i in range(num_states):
            d1 = 0
            for j in range(state_len / 2):
                d1 = d1 + states[i][j]

            d2 = 0
            for j in range(state_len / 2, state_len):
                d2 = d2 + states[i][j]

            ret.append(d1 - d2)

        return ret


def obj_function(states):

    num_states = len(states)
    state_len = len(states[0])
    ret = []
    for i in range(num_states):
        d1 = 0
        for j in range(state_len / 2):
            d1 = d1 + states[i][j]

        d2 = 0
        for j in range(state_len / 2, state_len):
            d2 = d2 + states[i][j]

        ret.append(d1 - d2)

    # return value can be a list or tuple
    return ret
    #return tuple(ret)


def solve_qsage_example():

    # use a local solver
    solver = local_connection.get_solver("c4-sw_sample")

    num_vars = 12

    # the first parameter is an objective function, the second parameter is
    # the number of variables, the third parameter is the solver, the fourth
    # parameter is the parameters for solver, the fifth parameter is the
    # parameters for qsage
    answer = solve_qsage(ObjClass(), num_vars, solver, {}, {"verbose": 2})
    print "solve_qsage ObjClass() answer: ", answer

    answer = solve_qsage(obj_function, num_vars, solver, {}, {"verbose": 2})
    print "solve_qsage obj_function answer: ", answer


if __name__ == "__main__":
    if len(sys.argv) == 1:
        solve_qsage_example()
    else:
        print "Usage: "
        print "%s: Minimize objective function with QSage" % sys.argv[0]
