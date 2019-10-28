# Copyright © 2019 D-Wave Systems Inc.
# The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

# Proprietary Information D-Wave Systems Inc.
# Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
# Notice this code is licensed to authorized users only under the
# applicable license agreement see eula.txt
# D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.

from dwave_sapi2.local import local_connection
from dwave_sapi2.remote import RemoteConnection
from dwave_sapi2.core import async_solve_ising, await_completion
import sys

#	This example shows how display local/remote solver properties as well as to use the solver to
#	asynchronously solve an ising problem by using async_solve_ising function. It also shows how
#	to cancel a submitted problem by using the cancel() function.
#
# usage:
#  1. list all solver names of local connection
#     connecting_to_solver

#  2. list solver properties of local connection and solve problem
#     connecting_to_solver solver_name

#  3. list all solver names of remote connection
#     connecting_to_solver url token

#  4. list all solver properties of remote connection and solve problem
#     connecting_to_solver url token solver_name


# list the solvers on a local connection
def list_local_solvers():
    solver_names = local_connection.solver_names()
    print "solvers' names: ", solver_names

# return local solver specified in first argument
def connect_to_local():
    solver = local_connection.get_solver(sys.argv[1])
    return solver

# list the solvers on a remote connection
def list_remote_solvers():
    remote_connection = RemoteConnection(sys.argv[1], sys.argv[2])
    solver_names = remote_connection.solver_names()
    print "solvers' names: ", solver_names

# return remote solver specified in first argument
def connect_to_remote():
    remote_connection = RemoteConnection(sys.argv[1], sys.argv[2])
    solver = remote_connection.get_solver(sys.argv[3])
    return solver

# display solver properties and solve two simple ising problems asynchronously
def get_properties_and_solve(solver):

    # problem 1.
    h1 = [0] * solver.properties["num_qubits"]
    h1[solver.properties["qubits"][0]] = 1
    J1 = {(i, j): -1 for i, j in solver.properties["couplers"]}

    # problem 2
    h2 = [0] * solver.properties["num_qubits"]
    J2 = {(i, j): 1 for i, j in solver.properties["couplers"]}

    # get solver's properties
    print "Solver's properties: ", solver.properties
    print "Number of qubits: ", solver.properties["num_qubits"]
    print "Working qubits: ", solver.properties["qubits"]
    print "Working couplers: ", solver.properties["couplers"]

    # solve Ising problems asynchronously
    answer1 = async_solve_ising(solver, h1, J1, num_reads=10)
    
    if "num_spin_reversal_transforms" in solver.properties["parameters"]:
        if "postprocess" in solver.properties["parameters"]:
            answer2 = async_solve_ising(solver, h2, J2, num_reads=20, postprocess='sampling', num_spin_reversal_transforms=1)
            submitted_problems = [answer1, answer2]
        else:
            print "Solver does not support postprocessing"
            submitted_problems = [answer1]

    else:
        print "Solver does not support spin reversal transforms"
        submitted_problems = [answer1]

    # wait for 1 problem to finish, with a maximum timeout of 30 seconds
    done = await_completion(submitted_problems, 1, 30)

    # print completed problem results; cancel incomplete
    for problem in submitted_problems:
        if problem.done():
            try:
                print "answer:", problem.result()
            except Exception as e:
                print e.message

        else:
            problem.cancel()


if __name__ == "__main__":
    if len(sys.argv) == 1:
        list_local_solvers()
    elif len(sys.argv) == 2:
        get_properties_and_solve(connect_to_local())
    elif len(sys.argv) == 3:
        list_remote_solvers()
    elif len(sys.argv) == 4:
        get_properties_and_solve(connect_to_remote())
    else:
        print "Usage: "
        print "%s: Print local solvers" % sys.argv[0]
        print "%s solver_name: Solve local" % sys.argv[0]
        print "%s url token: Print remote solvers" % sys.argv[0]
        print "%s url token solver_name: Solve remote" % sys.argv[0]
