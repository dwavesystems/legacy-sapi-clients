# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.embedding import find_embedding, embed_problem, unembed_answer
from dwave_sapi2.util import get_hardware_adjacency
from dwave_sapi2.local import local_connection
from dwave_sapi2.core import solve_ising
import sys


# This example shows how to use find_embedding to embed a problem using embed_problem,
# solve it using a local solver and then unembed the answer with unembed_answer
#
# usage:
#  embedding_example


def embedding_example():

    # formulate k_6 structured graph
    h = [1, 1, 1, 1, 1, 1]
    J = {(i, j): 1 for i in range(6) for j in range(i)}

    solver = local_connection.get_solver("c4-sw_optimize")
    A = get_hardware_adjacency(solver)

    # find and print embeddings for problem graph
    embeddings = find_embedding(J, A, verbose=1)
    print "embeddings are: ", embeddings

    # embed the problem into solver graph
    (h0, j0, jc, new_emb) = embed_problem(h, J, embeddings, A)
    print "embedded problem result:\nj0: ", j0
    print "jc: ", jc

    # find unembedded results for chain strengths -0.5, -1.0, -2.0
    for chain_strength in (-0.5, -1.0, -2.0):
        # set chain strength values
        jc = dict.fromkeys(jc, chain_strength)

        # create new J array concatenating j0 with jc
        emb_j = j0.copy()
        emb_j.update(jc)

        # solve embedded problem
        answer = solve_ising(solver, h0, emb_j, num_reads=10)

        # unembed and print result of the form:
        # solution [solution #]
        # var [var #] : [var value] ([qubit index] : [original qubit value] ...)
        result = unembed_answer(answer['solutions'], new_emb, broken_chains="minimize_energy", h=h, j=J)
        print "result for chain strength = ", chain_strength
        for i, (embsol, sol) in enumerate(zip(answer['solutions'], result)):
            print "solution", i
            for j, emb in enumerate(embeddings):
                print "var %d: %d (" % (j, sol[j]),
                for k in emb:
                    print "%d:%d" % (k, embsol[k]),
                print ")"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        embedding_example()
    else:
        print "Usage: "
        print "%s: Find embedding for k_6 structured graph, embed problem, solve problem, unembed answer" % sys.argv[0]
