# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.util import make_quadratic, reduce_degree
import sys

# This example shows how to use make_quadratic to generate an equivalent qubo representation
# from a function defined over binary variables as well as the reduce_degree function to reduce the
# greater than pairwise terms to pairwise by introducing new variables.
#
# usage:
#  reduce_degree_make_quadratic_example


def reduce_order_interaction():

    # A function f defined over binary variables represented as a
    # vector stored in decimal order
    #
    # f(x4, x3, x2, x1) =   a
    #                     + b * x1
    #                     + c * x2
    #                     + d * x3
    #                     + e * x4
    #                     + g * x1 * x2
    #                     + h * x1 * x3
    #                     + i * x1 * x4
    #                     + j * x2 * x3
    #                     + k * x2 * x4
    #                     + l * x3 * x4
    #                     + m * x1 * x2 * x3
    #                     + n * x1 * x2 * x4
    #                     + o * x1 * x3 * x4
    #                     + p * x2 * x3 * x4
    #                     + q * x1 * x2 * x3 * x4
    #
    # f(0000) means when x4 = 0, x3 = 0, x2 = 0, x1 = 0, so f(0000) = a
    # f(0001) means when x4 = 0, x3 = 0, x2 = 0, x1 = 1, so f(0001) = a + b
    # f(0010) means when x4 = 0, x3 = 0, x2 = 1, x1 = 0, so f(0010) = a + c
    # etc.
    f = [0, -3, 0, 2, 1, -4, 0, 5, 1, 2, 0, 2, 1, 0, -3, -1]

    # new_terms means terms after using ancillary variables
    # vars_rep means ancillary variables
    (Q, new_terms, vars_rep) = make_quadratic(f)
    print "Q: ", Q
    print "new_terms: ", new_terms
    print "vars_rep: ", vars_rep

    # f =   x1 * x2
    #     + x3 * x4
    terms = [[1, 2], [3, 4]]
    (new_terms, vars_rep) = reduce_degree(terms)
    print "new_terms: ", new_terms
    print "vars_rep", vars_rep

    # f =   x2 * x3 * x4 * x5 * x8
    #     + x3 * x6 * x8
    #     + x1 * x6 * x7 * x8
    #     + x2 * x3 * x5 * x6 * x7
    #     + x1 * x3 * x6
    #     + x1 * x6 * x8 * x10 * x12
    terms = [[2, 3, 4, 5, 8], [3, 6, 8], [1, 6, 7, 8], [2, 3, 5, 6, 7], [1, 3, 6], [1, 6, 8, 10, 12]]
    (new_terms, vars_rep) = reduce_degree(terms)
    print "new_terms: ", new_terms
    print "vars_rep", vars_rep

if __name__ == "__main__":
    if len(sys.argv) == 1:
        reduce_order_interaction()
    else:
        print "Usage: "
        print "%s: Reduce degree and make quadratic" % sys.argv[0]
