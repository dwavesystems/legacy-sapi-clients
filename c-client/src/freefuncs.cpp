//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "dwave_sapi.h"
#include "remote.hpp"

DWAVE_SAPI void sapi_freeConnection(sapi_Connection* connection) {
  if (dynamic_cast<sapi::RemoteConnection*>(connection)) delete connection;
}

DWAVE_SAPI void sapi_freeSolver(sapi_Solver*) {
  // deprecated; no effect
}

DWAVE_SAPI void sapi_freeSubmittedProblem(sapi_SubmittedProblem* submitted_problem) {
  delete submitted_problem;
}


DWAVE_SAPI void sapi_freeProblem(sapi_Problem* problem) {
  if (problem) {
    delete[] problem->elements;
    delete problem;
  }
}

DWAVE_SAPI void sapi_freeIsingResult(sapi_IsingResult* result) {
  if (result) {
    delete[] result->solutions;
    delete[] result->energies;
    delete[] result->num_occurrences;
    delete result;
  }
}

DWAVE_SAPI void sapi_freeEmbeddings(sapi_Embeddings* embeddings) {
  if (embeddings) {
    delete[] embeddings->elements;
    delete embeddings;
  }
}


DWAVE_SAPI void sapi_freeQSageResult(sapi_QSageResult* qsage_result) {
  if (qsage_result) {
    delete[] qsage_result->best_solution;
    delete[] qsage_result->info.progress.elements;

    delete qsage_result;
  }
}

DWAVE_SAPI void sapi_freeTerms(sapi_Terms* terms) {
  if (terms) {
    for (size_t i = 0; i < terms->len; ++i) delete[] terms->elements[i].terms;
    delete[] terms->elements;
    delete terms;
  }
}

DWAVE_SAPI void sapi_freeVariablesRep(sapi_VariablesRep* variables_rep) {
  if (variables_rep) {
    delete[] variables_rep->elements;
    delete variables_rep;
  }
}
