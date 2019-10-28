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

#include "dwave_sapi.h"

sapi_Code objective_function(const int* states, size_t len, size_t num_states, void* extra_arg, double* result)
{
	int state_len = len / num_states;
	int i, j;
	double d1, d2;
	for (i = 0; i < num_states; ++i)
	{
		d1 = 0.0;
		for (j = i * state_len; j < i * state_len + state_len / 2; ++j)
			d1 += states[j];

		d2 = 0.0;
		for (j = i * state_len + state_len / 2; j < (i + 1) * state_len; ++j)
			d2 += states[j];

		result[i] = d1 - d2;
	}

	return SAPI_OK;
}

/*
This example shows how to use sapi_solveQSage function to heuristically minimize arbitrary objective functions.

usage:
  sapi_solveQSage_example
*/
int main(void)
{
	sapi_Connection* connection = NULL;
	sapi_Solver* local_solver = NULL;
	sapi_QSageObjFunc qsageObjFunc;
	sapi_SwSampleSolverParameters solver_params = SAPI_SW_SAMPLE_SOLVER_DEFAULT_PARAMETERS;
	sapi_QSageParameters qsage_params = SAPI_QSAGE_DEFAULT_PARAMETERS;
	sapi_QSageResult* qsage_result = NULL;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
	int i;

	code = sapi_globalInit();

	if(code != SAPI_OK)
	{
		printf("Memory allocation failed!\n");
		return 1;
	}

	qsageObjFunc.objective_function = objective_function;
	qsageObjFunc.num_vars = 30;
	qsageObjFunc.objective_function_extra_arg = NULL;

	qsage_params.timeout = 20.0;
	qsage_params.verbose = 2;
	/* set qsage_params.ising_qubo to be SAPI_PROBLEM_TYPE_QUBO if the objective function takes variables 0/1, default is SAPI_PROBLEM_TYPE_ISING means the objective
	   function takes variables -1/1 */
	/* qsage_params.ising_qubo = SAPI_PROBLEM_TYPE_QUBO; */

	/* local connection */
	connection = sapi_localConnection();

	/* get a solver */
	local_solver = sapi_getSolver(connection, "c4-sw_sample");

	if (local_solver == NULL)
	{
		sapi_globalCleanup();
		return 1;
	}

	/* solve qsage problem */
	code = sapi_solveQSage(&qsageObjFunc, local_solver, (sapi_SolverParameters*)&solver_params, &qsage_params, &qsage_result, err_msg);

	if (code != SAPI_OK)
	{
		printf("%s\n", err_msg);
		sapi_freeSolver(local_solver);
		sapi_globalCleanup();
		return 1;
	}

	/* print qsage result */
	printf("%s\n", "qsage result:");
	printf("%s\n", "solution:");
	for (i = 0; i < qsage_result->len; ++i)
		printf("%d ", qsage_result->best_solution[i]);
	printf("\n");
	printf("best energy:\n%f\n", qsage_result->best_energy);

	printf("total number of state evaluations:\n%lld\n", qsage_result->info.stat.num_state_evaluations);
	printf("total number of objective function calls:\n%lld\n", qsage_result->info.stat.num_obj_func_calls);
	printf("total number of solver calls:\n%lld\n", qsage_result->info.stat.num_solver_calls);
	printf("total number of lp solver calls:\n%lld\n", qsage_result->info.stat.num_lp_solver_calls);
	printf("state evaluations time:\n%f\n", qsage_result->info.state_evaluations_time);
	printf("solver calls time:\n%f\n", qsage_result->info.solver_calls_time);
	printf("lp solver calls time:\n%f\n", qsage_result->info.lp_solver_calls_time);
	printf("sapi_solveQSage total running time:\n%f\n", qsage_result->info.total_time);

	printf("%s\n", "qsage computation progress:");
	for (i = 0; i < qsage_result->info.progress.len; ++i)
		printf("[%lld %lld %lld %lld %f %f]\n", qsage_result->info.progress.elements[i].stat.num_state_evaluations,
		                                        qsage_result->info.progress.elements[i].stat.num_obj_func_calls,
												qsage_result->info.progress.elements[i].stat.num_solver_calls,
												qsage_result->info.progress.elements[i].stat.num_lp_solver_calls,
												qsage_result->info.progress.elements[i].time,
												qsage_result->info.progress.elements[i].energy);

	/* free memory */
	sapi_freeSolver(local_solver);
	sapi_freeQSageResult(qsage_result);

	sapi_globalCleanup();

	return 0;
}
