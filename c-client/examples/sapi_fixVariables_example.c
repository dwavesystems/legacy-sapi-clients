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

/*
This example shows how to use sapi_fixVariables to fix variables for an ising problem.

usage:
  sapi_fixVariables_example
*/

int main(void)
{
	sapi_Problem problem;
	sapi_FixVariablesMethod fix_variables_method;
	sapi_FixVariablesResult* fix_variables_result = NULL;
	int num_variables = 3;
	sapi_Code code;
	int i;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];

	/* construct problem data */
	problem.len = num_variables + 1;
	problem.elements = (sapi_ProblemEntry*)malloc(sizeof(sapi_ProblemEntry) * problem.len);

	/* set variable 0 to 1.0 */
	problem.elements[0].i = 0;
	problem.elements[0].j = 0;
	problem.elements[0].value = 1.0;

	/* set variable 1 to 1.0 */
	problem.elements[1].i = 1;
	problem.elements[1].j = 1;
	problem.elements[1].value = 1.0;

	/* set variable 2 to -1.0 */
	problem.elements[2].i = 2;
	problem.elements[2].j = 2;
	problem.elements[2].value = -1.0;

	/* set edge(0,1) to -2.0 */
	problem.elements[3].i = 0;
	problem.elements[3].j = 1;
	problem.elements[3].value = -2.0;

	/* fix variables twice; once standard, once optimized */
	/* standard */
	fix_variables_method = SAPI_FIX_VARIABLES_METHOD_STANDARD;
	code = sapi_fixVariables(&problem, fix_variables_method, &fix_variables_result, err_msg);

	if (code != SAPI_OK)
	{
		free(problem.elements);
		return 1;
	}

	for (i = 0; i < fix_variables_result->fixed_variables_len; ++i)
		printf("variable %d has been fixed to be value: %d\n", fix_variables_result->fixed_variables[i].var, fix_variables_result->fixed_variables[i].value);

	for (i = 0; i < fix_variables_result->new_problem.len; ++i)
		printf("[%d][%d]: %f\n", fix_variables_result->new_problem.elements[i].i, fix_variables_result->new_problem.elements[i].j, fix_variables_result->new_problem.elements[i].value);

	printf("offset is: %f\n\n", fix_variables_result->offset);

	sapi_freeFixVariablesResult(fix_variables_result);

	/* optimized */
	fix_variables_method = SAPI_FIX_VARIABLES_METHOD_OPTIMIZED;
	code = sapi_fixVariables(&problem, fix_variables_method, &fix_variables_result, err_msg);

	if (code != SAPI_OK)
	{
		free(problem.elements);
		return 1;
	}

	for (i = 0; i < fix_variables_result->fixed_variables_len; ++i)
		printf("variable %d has been fixed to be value: %d\n", fix_variables_result->fixed_variables[i].var, fix_variables_result->fixed_variables[i].value);

	for (i = 0; i < fix_variables_result->new_problem.len; ++i)
		printf("[%d][%d]: %f\n", fix_variables_result->new_problem.elements[i].i, fix_variables_result->new_problem.elements[i].j, fix_variables_result->new_problem.elements[i].value);

	printf("offset is: %f\n\n", fix_variables_result->offset);

	/* free memory */
	sapi_freeFixVariablesResult(fix_variables_result);
	free(problem.elements);

	return 0;
}
