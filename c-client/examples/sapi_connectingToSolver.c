//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

/*
Proprietary Information D-Wave Systems Inc.
Copyright (c) 2015 by D-Wave Systems Inc. All rights reserved.
Notice this code is licensed to authorized users only under the
applicable license agreement see eula.txt
D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
*/

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dwave_sapi.h"

/*
	This example shows how display remote solver properties as well as to use the solver to
	asynchronously solve an ising problem by using sapi_asyncSolveIsing function. It also shows how
	to cancel a submitted problem by using sapi_cancelSubmittedProblem function.



usage:
  1. list all solver names of remote connection:
     sapi_connectingToSolver url token

  2. list all solver properties of remote connection and solve problem:
   	 sapi_connectingToSolver url token solver_name
*/

int main(int argc, const char* argv[])
{
	sapi_Connection* connection = NULL;
	const char** solver_names = NULL;
	const char* solver_name = NULL;
	const char* proxy_url = NULL;
	sapi_Solver* solver = NULL;
	const sapi_SolverProperties* solver_properties = NULL;
	sapi_Problem problems[2] = {{NULL, 0}, {NULL, 0}};
	sapi_QuantumSolverParameters params = SAPI_QUANTUM_SOLVER_DEFAULT_PARAMETERS;
	/* submitted_problems stores the submitted problems */
	sapi_SubmittedProblem* submitted_problems[2] = {NULL, NULL};
	/* results stores the submitted problems' results */
	sapi_IsingResult* results[2] = {NULL, NULL};
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
	int i, j, n, k;
	double timeout = 30.0;
	int done = 0;
	int ret = 1;

	code = sapi_globalInit();

	if(code != SAPI_OK)
	{
		printf("Memory allocation failed!\n");
		return 1;
	}

	if (argc == 3 || argc == 4) /* remote connection */
	{
		if (argc == 4)
			solver_name = argv[3];
		code = sapi_remoteConnection(argv[1], argv[2], proxy_url, &connection, err_msg);
		if (code != SAPI_OK)
		{
			printf("%s\n", err_msg);
			goto CLEANUP;
		}
	}
	else
	{
		printf("%s\n", "must have 2 or 3 arguments: url token [solver_name]");
		goto CLEANUP;
	}

	/* get solver */
	if (argc == 4) solver = sapi_getSolver(connection, solver_name);

	if (solver == NULL)
	{
		/* list all solver names */
		solver_names = sapi_listSolvers(connection);

		/* print solver names */
		printf("%s\n", "solver_names:");
		for (i = 0; solver_names[i] != NULL; ++i)
			printf("%s\n", solver_names[i]);

		ret = 0;
		goto CLEANUP;
	}

	/* get solver properties */
	solver_properties = sapi_getSolverProperties(solver);

	/* print available solver properties */
	if (solver_properties->supported_problem_types != NULL)
	{
		printf("\"%s\" solver's supported problem type properties:\n", solver_name);
		for (i = 0; i < solver_properties->supported_problem_types->len; ++i)
			printf("%s\n", solver_properties->supported_problem_types->elements[i]);
	}

	if (solver_properties->quantum_solver != NULL)
	{
		printf("\"%s\" solver's quantum solver properties:\n", solver_name);
		printf("num_qubits = %d\n", solver_properties->quantum_solver->num_qubits);
		printf("%s\n", "qubits:");
		for (i = 0; i < solver_properties->quantum_solver->qubits_len; ++i)
			printf("%d ", solver_properties->quantum_solver->qubits[i]);
		printf("\n");
		printf("%s\n", "couplers:");
		for (i = 0; i <solver_properties->quantum_solver->couplers_len; ++i)
			printf("[%d, %d] ", solver_properties->quantum_solver->couplers[i].q1, solver_properties->quantum_solver->couplers[i].q2);
		printf("\n");
	}

	if (solver_properties->ising_ranges != NULL)
	{
		printf("\"%s\" solver's ising range properties:\n", solver_name);
		printf("h range: [%f, %f], j range: [%f, %f]\n", solver_properties->ising_ranges->h_min,
			                                             solver_properties->ising_ranges->h_max,
														 solver_properties->ising_ranges->j_min,
														 solver_properties->ising_ranges->j_max);
	}

	/* problem type 0 */
	n = 0;
	problems[n].len = solver_properties->quantum_solver->couplers_len + 1;
	problems[n].elements = malloc(problems[n].len * sizeof(sapi_ProblemEntry));
	for(j = 0; j < solver_properties->quantum_solver->couplers_len; j++)
	{
		problems[n].elements[j].i = solver_properties->quantum_solver->couplers[j].q1;
		problems[n].elements[j].j = solver_properties->quantum_solver->couplers[j].q2;
		problems[n].elements[j].value = -1;
	}
	problems[n].elements[solver_properties->quantum_solver->couplers_len].i = solver_properties->quantum_solver->qubits[0];
	problems[n].elements[solver_properties->quantum_solver->couplers_len].j = solver_properties->quantum_solver->qubits[0];
	problems[n].elements[solver_properties->quantum_solver->couplers_len].value = 1;

	/* problem type 1 */
	n = 1;
	problems[n].len = solver_properties->quantum_solver->couplers_len;
	problems[n].elements = malloc(problems[n].len * sizeof(sapi_ProblemEntry));
	for(j = 0; j < solver_properties->quantum_solver->couplers_len; j++)
	{
		problems[n].elements[j].i = solver_properties->quantum_solver->couplers[j].q1;
		problems[n].elements[j].j = solver_properties->quantum_solver->couplers[j].q2;
		problems[n].elements[j].value = 1;
	}

	/* solve problem 0 with parameter num_reads=10 */
	params.num_reads = 10;

	code = sapi_asyncSolveIsing(solver, &problems[0], (sapi_SolverParameters*)&params, &submitted_problems[0], err_msg);

	if (code != SAPI_OK)
	{
		printf("%s\n", err_msg);
		goto CLEANUP;
	}

	/* solve problem 1 with parameter num_reads=20, postprocess=sampling and num_spin_reversal_transforms=1 */
	params.num_reads = 20;
	params.postprocess = SAPI_POSTPROCESS_SAMPLING;
	params.num_spin_reversal_transforms = 1;

	code = sapi_asyncSolveIsing(solver, &problems[1], (sapi_SolverParameters*)&params, &submitted_problems[1], err_msg);

	if (code != SAPI_OK)
	{
		printf("%s\n", err_msg);
		goto CLEANUP;
	}

	/* await submitted problem completion */
	sapi_awaitCompletion((const sapi_SubmittedProblem **)submitted_problems, 2, 1, timeout);

	/* retrieve the answer from an ansync result */
	for (k = 0; k < 2; ++k)
	{
		/* cancel submitted problems that are not done */
		done = sapi_asyncDone(submitted_problems[k]);
		if(!done)
		{
			sapi_cancelSubmittedProblem(submitted_problems[k]);
			printf("%s%d%s\n", "problem ", k, " cancelled");
		}
		else
		{
			code = sapi_asyncResult(submitted_problems[k], &results[k], err_msg);

			if (code != SAPI_OK)
				printf("%s\n", err_msg);
			else
			{
				/* print result */
				printf("%s%d%s\n", "result from problem ", k, ":");
				for (i = 0; i < results[k]->num_solutions; ++i)
				{
					printf("%s%d%s\n", "solution ", i, ":");
					for (j = 0; j < results[k]->solution_len; ++j)
						printf("%d ", results[k]->solutions[i * results[k]->solution_len + j]);
					printf("\n");
					printf("energy:\n%f\n", results[k]->energies[i]);
					if (results[k]->num_occurrences != NULL)
						printf("num_occurrences:\n%d\n", results[k]->num_occurrences[i]);
					printf("\n");
				}
			}
		}
	}

	/* if finished correctly, set ret to 0 */
	ret = 0;

	/* free memory */
	CLEANUP:
	sapi_freeConnection(connection);
	sapi_freeSolver(solver);
	for (i = 0; i < 2; ++i)
	{
		sapi_freeSubmittedProblem(submitted_problems[i]);
		sapi_freeIsingResult(results[i]);
		free(problems[i].elements);
	}

	sapi_globalCleanup();

	return ret;
}
