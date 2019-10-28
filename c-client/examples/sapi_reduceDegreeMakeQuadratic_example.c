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

#include "dwave_sapi.h"

/*
This example shows how to use sapi_reduceDegree function to reduce the greater than pairwise terms to pairwise by introducing new variables.
as well as how to use sapi_makeQuadratic to generate an equivalent qubo representation from a function defined over binary variables.


usage:
  sapi_reduceDegreeMakeQuadratic_example
*/

int main(void)
{
	sapi_Terms* terms = (sapi_Terms*)malloc(sizeof(sapi_Terms));
	sapi_Terms* new_terms = NULL;
	sapi_VariablesRep* variables_rep = NULL;
	sapi_Problem* Q = NULL;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
	int i, j;
	/*
	f =   x0 * x2 * x3 * x4 * x5 * x8
        + x3 * x6 * x8
        + x1 * x6 * x7 * x8
        + x0 * x2 * x3 * x5 * x6 * x7
        + x1 * x3 * x6
        + x1 * x6 * x8 * x10 * x12
	*/
	int num_terms = 6;
	int terms_1[] = {0, 2, 3, 4, 5, 8};
	int terms_2[] = {3, 6, 8};
	int terms_3[] = {1, 6, 7, 8};
	int terms_4[] = {0, 2, 3, 5, 6, 7};
	int terms_5[] = {1, 3, 6};
	int terms_6[] = {1, 6, 8, 10, 12};
	int terms_len[] = {6, 3, 4, 6, 3, 5};
	int* t[6];

	/*
	A function f defined over binary variables represented as an
    array stored in decimal order

    f(x3, x2, x1, x0) =   a
                        + b * x0
                        + c * x1
                        + d * x2
                        + e * x3
                        + g * x0 * x1
                        + h * x0 * x2
                        + i * x0 * x3
                        + j * x1 * x2
                        + k * x1 * x3
                        + l * x2 * x3
                        + m * x0 * x1 * x2
                        + n * x0 * x1 * x3
                        + o * x0 * x2 * x3
                        + p * x1 * x2 * x3
                        + q * x0 * x1 * x2 * x3

    f(0000) means when x3 = 0, x2 = 0, x1 = 0, x0 = 0, so f(0000) = a
    f(0001) means when x3 = 0, x2 = 0, x1 = 0, x0 = 1, so f(0001) = a + b
    f(0010) means when x3 = 0, x2 = 0, x1 = 1, x0 = 0, so f(0010) = a + c
    etc.
	*/

	double f[] = {0, -1, 2, 1, 4, -1, 0, 0, -1, -3, 0, -1, 0, 3, 2, 2};
	int f_len = sizeof(f) / sizeof(double);


	/* reduce degree */
	t[0] = terms_1;
	t[1] = terms_2;
	t[2] = terms_3;
	t[3] = terms_4;
	t[4] = terms_5;
	t[5] = terms_6;

	terms->len = num_terms;
	terms->elements = (sapi_TermsEntry*)malloc(sizeof(sapi_TermsEntry) * num_terms);

	for (i = 0; i < num_terms; ++i)
	{
		terms->elements[i].len = terms_len[i];
		terms->elements[i].terms = (int*)malloc(sizeof(int) * terms_len[i]);
		for (j = 0; j < terms_len[i]; ++j)
			terms->elements[i].terms[j] = t[i][j];
	}

	code = sapi_reduceDegree(terms, &new_terms, &variables_rep, err_msg);

	if (code != SAPI_OK)
	{
		printf("%s\n", err_msg);
		for (i = 0; i < num_terms; ++i)
			free(terms->elements[i].terms);
		free(terms->elements);
		free(terms);
		return 1;
	}

	printf("new reduced degree terms are:\n");
	for (i = 0; i < new_terms->len; ++i)
	{
		for (j = 0; j < new_terms->elements[i].len; ++j)
			printf("%d ", new_terms->elements[i].terms[j]);
		printf("\n");
	}

	printf("variables rep are:\n");
	for (i = 0; i < variables_rep->len; ++i)
		printf("%d -> %d %d\n", variables_rep->elements[i].variable, variables_rep->elements[i].rep[0], variables_rep->elements[i].rep[1]);
	printf("\n");

	/* free memory */
	for (i = 0; i < num_terms; ++i)
		free(terms->elements[i].terms);
	free(terms->elements);
	free(terms);
	sapi_freeVariablesRep(variables_rep);

	/* make quadratic */
	code = sapi_makeQuadratic(f, f_len, NULL, &terms, &variables_rep, &Q, err_msg);

	if (code != SAPI_OK)
	{
		printf("%s\n", err_msg);
		return 1;
	}

	printf("new quadratic terms are:\n");
	for (i = 0; i < terms->len; ++i)
	{
		for (j = 0; j < terms->elements[i].len; ++j)
			printf("%d ", terms->elements[i].terms[j]);
		printf("\n");
	}

	printf("new variables rep are:\n");
	for (i = 0; i < variables_rep->len; ++i)
		printf("%d -> %d %d\n", variables_rep->elements[i].variable, variables_rep->elements[i].rep[0], variables_rep->elements[i].rep[1]);

	printf("Q is:\n");
	for (i = 0; i < Q->len; ++i)
		printf("Q[%d, %d] = %f\n", Q->elements[i].i, Q->elements[i].j, Q->elements[i].value);


	/* free memory */
	sapi_freeTerms(terms);
	sapi_freeTerms(new_terms);
	sapi_freeVariablesRep(variables_rep);
	sapi_freeProblem(Q);

	return 0;
}
