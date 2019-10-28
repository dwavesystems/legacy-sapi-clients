# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.fix_variables import fix_variables
import sys

# This example shows how to use fix_variables to fix variables for an ising problem.
#
# usage:
#  fix_variables_example


def fix_variables_example():
    # create QUBO
    Q = {(0, 0): 1, (1, 1): 1, (2, 2): -1, (0, 1): -2}

    # fix variables, standard method
    result = fix_variables(Q, method="standard")
    print "standard fixed variables result: ", result

    # fix variables, optimized method
    result = fix_variables(Q, method="optimized")
    print "optimized fixed variables result: ", result

if __name__ == "__main__":
    if len(sys.argv) == 1:
        fix_variables_example()
    else:
        print "Usage: "
        print "%s: Run fix variables on simple QUBO" % sys.argv[0]
