//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "gtest/gtest.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "dwave_sapi.h"


TEST(TEST_DWAVE_SAPI, test_sapi_findEmbedding_1)
{
	int num_variables = 10;
	sapi_Problem S;
	int M = 4;
	int N = M;
	int L = 4;
	sapi_Problem* A = NULL;
	sapi_Embeddings* embeddings = NULL;
	sapi_FindEmbeddingParameters params = SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;
	int i, j, index;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];

	sapi_globalInit();

	S.len = num_variables * num_variables;
	S.elements = (sapi_ProblemEntry*)malloc(sizeof(sapi_ProblemEntry) * S.len);
	index = 0;
	for (i = 0; i < num_variables; ++i)
		for (j = 0; j < num_variables; ++j)
		{
			S.elements[index].i = i;
			S.elements[index++].j = j;
		}

	code = sapi_getChimeraAdjacency(M, N, L, &A);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	code = sapi_findEmbedding(&S, A, &params, &embeddings, err_msg);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	free(S.elements);
	sapi_freeProblem(A);
	sapi_freeEmbeddings(embeddings);

	sapi_globalCleanup();
}

TEST(TEST_DWAVE_SAPI, test_sapi_findEmbedding_2)
{
	sapi_Connection* local_connection = NULL;
	sapi_Solver* local_solver = NULL;
	int num_variables = 10;
	sapi_Problem S;
	sapi_Problem* A = NULL;
	sapi_Embeddings* embeddings = NULL;
	sapi_FindEmbeddingParameters params = SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;
	int i, j, index;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];

	sapi_globalInit();

	local_connection = sapi_localConnection();
	ASSERT_TRUE(local_connection != NULL);
	if (HasFailure())
		return;

	local_solver = sapi_getSolver(local_connection, "c4-sw_sample");
	ASSERT_TRUE(local_solver != NULL);
	if (HasFailure())
		return;

	/* get hardware adjacency */
	code = sapi_getHardwareAdjacency(local_solver, &A);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	S.len = num_variables * num_variables;
	S.elements = (sapi_ProblemEntry*)malloc(sizeof(sapi_ProblemEntry) * S.len);
	index = 0;
	for (i = 0; i < num_variables; ++i)
		for (j = 0; j < num_variables; ++j)
		{
			S.elements[index].i = i;
			S.elements[index++].j = j;
		}

	code = sapi_findEmbedding(&S, A, &params, &embeddings, err_msg);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	sapi_freeSolver(local_solver);
	free(S.elements);
	sapi_freeProblem(A);
	sapi_freeEmbeddings(embeddings);

	sapi_globalCleanup();
}

TEST(TEST_DWAVE_SAPI, test_sapi_getChimeraAdjacency)
{
	int M = 4;
	int N = M;
	int L = 4;
	sapi_Problem* A = NULL;
	sapi_Code code;

	code = sapi_getChimeraAdjacency(M, N, L, &A);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	sapi_freeProblem(A);
}

TEST(TEST_DWAVE_SAPI, test_sapi_getHardwareAdjacency)
{
	sapi_Connection* local_connection = NULL;
	sapi_Solver* local_solver = NULL;
	sapi_Problem* A = NULL;
	sapi_Code code;

	sapi_globalInit();
	local_connection = sapi_localConnection();
	ASSERT_TRUE(local_connection != NULL);
	if (HasFailure())
		return;

	local_solver = sapi_getSolver(local_connection, "c4-sw_sample");
	ASSERT_TRUE(local_solver != NULL);
	if (HasFailure())
		return;

	/* get hardware adjacency */
	code = sapi_getHardwareAdjacency(local_solver, &A);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	sapi_freeSolver(local_solver);
	sapi_freeProblem(A);

	sapi_globalCleanup();
}


TEST(TEST_DWAVE_SAPI, test_sapi_makeQuadratic)
{
	sapi_Terms* terms = NULL;
	sapi_VariablesRep* variables_rep = NULL;
	sapi_Problem* Q = NULL;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
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

	code = sapi_makeQuadratic(f, f_len, NULL, &terms, &variables_rep, &Q, err_msg);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	/* free memory */
	sapi_freeTerms(terms);
	sapi_freeVariablesRep(variables_rep);
	sapi_freeProblem(Q);
}

TEST(TEST_DWAVE_SAPI, test_sapi_reduceDegree)
{
	sapi_Terms* terms = (sapi_Terms*)malloc(sizeof(sapi_Terms));
	sapi_Terms* new_terms = NULL;
	sapi_VariablesRep* variables_rep = NULL;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
	int i, j;
	/*
	f =   x2 * x3 * x4 * x5 * x8
        + x3 * x6 * x8
        + x1 * x6 * x7 * x8
        + x2 * x3 * x5 * x6 * x7
        + x1 * x3 * x6
        + x1 * x6 * x8 * x10 * x12
	*/
	int num_terms = 6;
	int terms_1[] = {2, 3, 4, 5, 8};
	int terms_2[] = {3, 6, 8};
	int terms_3[] = {1, 6, 7, 8};
	int terms_4[] = {2, 3, 5, 6, 7};
	int terms_5[] = {1, 3, 6};
	int terms_6[] = {1, 6, 8, 10, 12};
	int terms_len[] = {5, 3, 4, 5, 3, 5};
	int* t[6];

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

	// temporarily change the first element in the first term to be a negative number
	i = terms->elements[0].terms[0];
	terms->elements[0].terms[0] = -i;
	code = sapi_reduceDegree(terms, &new_terms, &variables_rep, err_msg);
	if (code != SAPI_ERR_INVALID_PARAMETER)
	{
		printf("%s\n", "test_sapi_reduceDegree sapi_reduceDegree error.");
		for (i = 0; i < num_terms; ++i)
			free(terms->elements[i].terms);
		free(terms->elements);
		free(terms);
		return;
	}

	// change it back
	terms->elements[0].terms[0] = i;

	code = sapi_reduceDegree(terms, &new_terms, &variables_rep, err_msg);
	ASSERT_TRUE(code == SAPI_OK);
	if (HasFailure())
		return;

	/* free memory */
	for (i = 0; i < num_terms; ++i)
		free(terms->elements[i].terms);
	free(terms->elements);
	free(terms);

	sapi_freeTerms(new_terms);
	sapi_freeVariablesRep(variables_rep);
}

TEST(TEST_DWAVE_SAPI, test_sapi_findEmbedding_3)
{
	int num_variables = 3;
	sapi_Problem S;
	sapi_Problem A;
	sapi_Embeddings* embeddings = NULL;
	sapi_FindEmbeddingParameters params = SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;
	int i, j, index;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];

	S.len = num_variables;
	sapi_ProblemEntry buffer1[3] = {{0,1,0.0}, {0,2,0.0}, {1,2,0.0}};
	S.elements = buffer1;

	A.len = num_variables;
	sapi_ProblemEntry buffer2[3] = {{0,1,0.0}, {1,2,0.0}, {2,3,0.0}};
	A.elements = buffer2;

	code = sapi_findEmbedding(&S, &A, &params, &embeddings, err_msg);

	EXPECT_EQ(SAPI_ERR_NO_EMBEDDING_FOUND, code);
	EXPECT_EQ(NULL, embeddings);
	
	std::string message = "Failed to find embedding.";
	EXPECT_EQ(message, std::string(err_msg));

}

TEST(TEST_DWAVE_SAPI, test_sapi_findEmbedding_4)
{
	int num_variables = 3;
	sapi_Problem S;
	sapi_Problem A;
	sapi_Embeddings* embeddings = NULL;
	sapi_FindEmbeddingParameters params = SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;
	int i, j, index;
	sapi_Code code;
	char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];

	//empty problem
	S.len = 0;

	//any solution array initialized.
	A.len = num_variables;
	sapi_ProblemEntry buffer2[3] = {{0,1,0.0}, {1,2,0.0}, {2,3,0.0}};
	A.elements = buffer2;

	code = sapi_findEmbedding(&S, &A, &params, &embeddings, err_msg);

	EXPECT_EQ(SAPI_OK, code);
	EXPECT_EQ(0, embeddings->len);

	sapi_freeEmbeddings(embeddings);
}

