//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

/*
Proprietary Information D-Wave Systems Inc.
Copyright (c) 2016 by D-Wave Systems Inc. All rights reserved.
Notice this code is licensed to authorized users only under the
applicable license agreement see eula.txt
D-Wave Systems Inc., 3033 Beta Ave., Burnaby, BC, V5G 4M9, Canada.
*/

#include <stdio.h>
#include <stdlib.h>

#include "dwave_sapi.h"

/*
This example shows how to use sapi_embedProblem to embed a problem.

usage:
  sapi_embedding_example
*/

const size_t num_variables = 5;
sapi_ProblemEntry entries[] = {
    {0, 1, 1.0}, {0, 2, 1.0}, {0, 3, 1.0}, {0, 4, 1.0}, {1, 2, 1.0},
    {1, 3, 1.0}, {1, 4, 1.0}, {2, 3, 1.0}, {2, 4, 1.0}, {3, 4, 1.0}
};
sapi_Problem problem = {entries, sizeof(entries) / sizeof(entries[0])};

int main(void) {
    sapi_Solver *solver = NULL;
    sapi_Problem *adj = NULL;
    sapi_Embeddings *embeddings = NULL;
    sapi_FindEmbeddingParameters find_embedding_params = SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS;
    sapi_Problem embedded_problem = {NULL, 0};
    sapi_EmbedProblemResult *embed_problem_result = NULL;
    sapi_SwOptimizeSolverParameters solver_params = SAPI_SW_OPTIMIZE_SOLVER_DEFAULT_PARAMETERS;
    sapi_IsingResult *answer = NULL;
    size_t i, j, k;
    int c, first;
    int *new_solutions = NULL;
    int *nsr;
    double chain_strength;
    size_t num_new_solutions;
    sapi_Code code;
    char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
    int ret = 1;

    code = sapi_globalInit();
    if(code != SAPI_OK) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    /* get a solver */
    solver = sapi_getSolver(sapi_localConnection(), "c4-sw_optimize");
    if (solver == NULL) {
        printf("Couldn't find solver\n");
        goto CLEANUP;
    }

    /* get hardware adjacency */
    code = sapi_getHardwareAdjacency(solver, &adj);
    if (code != SAPI_OK) {
        printf("Couldn't get hardware adjacency\n");
        goto CLEANUP;
    }

    /* find the embedding */
    find_embedding_params.verbose = 1;
    code = sapi_findEmbedding(&problem, adj, &find_embedding_params, &embeddings, err_msg);
    if (code != SAPI_OK) {
        printf("%s\n", err_msg);
        goto CLEANUP;
    }

    /* embed the problem */
    code = sapi_embedProblem(&problem, embeddings, adj, 0, 0, NULL, &embed_problem_result, err_msg);
    if (code != SAPI_OK) {
        printf("%s\n", err_msg);
        goto CLEANUP;
    }

    /* print embedded problem result */
    for (i = 0; i < embed_problem_result->problem.len; ++i) {
        printf("problem[%d][%d] = %f\n",
            embed_problem_result->problem.elements[i].i,
            embed_problem_result->problem.elements[i].j,
            embed_problem_result->problem.elements[i].value);
    }
    printf("\n");

    for (i = 0; i < embed_problem_result->jc.len; ++i) {
        printf("jc[%d][%d] = %f\n",
            embed_problem_result->jc.elements[i].i,
            embed_problem_result->jc.elements[i].j,
            embed_problem_result->jc.elements[i].value);
    }

    /* allocate space for new embedded problem result */
    embedded_problem.len = embed_problem_result->problem.len + embed_problem_result->jc.len;
    embedded_problem.elements = malloc(sizeof(sapi_ProblemEntry) * embedded_problem.len);
    if (embedded_problem.elements == NULL) {
        printf("embedded_problem.elements allocation failed\n");
        goto CLEANUP;
    }

    /* store embedded problem result in new problem */
    for(i = 0; i < embed_problem_result->problem.len; i++) {
        embedded_problem.elements[i].i = embed_problem_result->problem.elements[i].i;
        embedded_problem.elements[i].j = embed_problem_result->problem.elements[i].j;
        embedded_problem.elements[i].value = embed_problem_result->problem.elements[i].value;
    }
    for(; i < embedded_problem.len; i++) {
        embedded_problem.elements[i].i = embed_problem_result->jc.elements[i - embed_problem_result->problem.len].i;
        embedded_problem.elements[i].j = embed_problem_result->jc.elements[i - embed_problem_result->problem.len].j;
        /* set value differently in each loop iteration below */
    }

    /* main for loop encapsulating embedded problems of varying chain strengths, solving, then unembedding and displaying result */
    for(c = 0; c < 3; c++) {

        /* adjust chain strengths */
        switch (c) {
            case 0: chain_strength = -0.5; break;
            case 1: chain_strength = -1.0; break;
            default: chain_strength = -2.0; break;
        }
        for(i = embed_problem_result->problem.len; i < embedded_problem.len; i++) {
            embedded_problem.elements[i].value = chain_strength;
        }

        /* solve embedded problem */
        solver_params.num_reads = 10;
        if (answer != NULL) {
            sapi_freeIsingResult(answer);
            answer = NULL;
        }
        code = sapi_solveIsing(solver, &embedded_problem, (sapi_SolverParameters*)&solver_params, &answer, err_msg);
        if (code != SAPI_OK) {
            printf("%s\n", err_msg);
            goto CLEANUP;
        }

        nsr = realloc(new_solutions, answer->num_solutions * num_variables * sizeof(int*));
        if (nsr == NULL) {
            printf("new_solutions allocation failed\n");
            goto CLEANUP;
        }
        new_solutions = nsr;

        code = sapi_unembedAnswer(answer->solutions, answer->solution_len, answer->num_solutions, embeddings,
            SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY, &problem, new_solutions, &num_new_solutions, err_msg);
        if (code != SAPI_OK) {
            printf("%s\n", err_msg);
            goto CLEANUP;
        }

        /* print unembedded problem result of the form:
        solution [solution #]
        var [var #] : [var value] ([qubit index] : [original qubit value] ...) */
        printf("result for chain strength = %.1f:\n", chain_strength);
        for (i = 0; i < num_new_solutions; ++i) {
            printf("solution %d:\n", (int)i);
            for (j = 0; j < num_variables; ++j) {
                first = 1;
                printf("var %d: %d (", (int)j, new_solutions[i * num_variables + j]);
                for (k = 0; k < embeddings->len; k++) {
                    if (embeddings->elements[k] == j) {
                        if(first) {
                            printf("%d: %d", (int)k, answer->solutions[i * answer->solution_len + k]);
                            first = 0;
                        } else {
                            printf(", %d: %d", (int)k, answer->solutions[i * answer->solution_len + k]);
                        }
                    }
                }
                printf(")\n");
            }
            printf("\n");
        }
    }

    /* if finished correctly, set ret to 0 */
    ret = 0;
    /* free memory */
    CLEANUP:
    sapi_freeSolver(solver);
    sapi_freeProblem(adj);
    sapi_freeEmbeddings(embeddings);
    sapi_freeEmbedProblemResult(embed_problem_result);
    free(embedded_problem.elements);
    sapi_freeIsingResult(answer);
    free(new_solutions);
    sapi_globalCleanup();

    return ret;
}
