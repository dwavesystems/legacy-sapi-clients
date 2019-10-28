//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <limits>
#include <exception>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/random.hpp>

#include "find_embedding.hpp"
#include "blackbox.hpp"

#include "dwave_sapi.h"
#include "conversions.h"
#include "internal.hpp"
#include <sapi-impl.hpp>

using std::vector;
using std::current_exception;

using sapi::handleException;
using sapi::writeErrorMessage;

namespace
{

/**
* \brief find embedding parameter default values
*/
const int SAPI_FIND_EMBEDDING_DEFAULT_FAST_EMBEDDING = std::numeric_limits<int>::min();
const int SAPI_FIND_EMBEDDING_DEFAULT_MAX_NO_IMPROVEMENT = std::numeric_limits<int>::min();
const int SAPI_FIND_EMBEDDING_DEFAULT_USE_RANDOM_SEED = 0;
const int SAPI_FIND_EMBEDDING_DEFAULT_RANDOM_SEED = 0;
const double SAPI_FIND_EMBEDDING_DEFAULT_TIMEOUT = -std::numeric_limits<double>::max();
const int SAPI_FIND_EMBEDDING_DEFAULT_TRIES = std::numeric_limits<int>::min();
const int SAPI_FIND_EMBEDDING_DEFAULT_VERBOSE = std::numeric_limits<int>::min();

/**
* \brief qsage parameter default values
*/
const int SAPI_QSAGE_DEFAULT_DRAW_SAMPLE = std::numeric_limits<int>::min();
const double SAPI_QSAGE_DEFAULT_EXIT_THRESHOLD_VALUE = -std::numeric_limits<double>::infinity();
const int* SAPI_QSAGE_DEFAULT_INITIAL_SOLUTION = NULL;
sapi_ProblemType SAPI_QSAGE_DEFAULT_ISING_QUBO = SAPI_PROBLEM_TYPE_ISING;
sapi_QSageLPSolver SAPI_QSAGE_DEFAULT_LP_SOLVER = NULL;
void* SAPI_QSAGE_DEFAULT_LP_SOLVER_EXTRA_ARG = NULL;
const long long int SAPI_QSAGE_DEFAULT_MAX_NUM_STATE_EVALUATIONS = std::numeric_limits<long long int>::min();
const int SAPI_QSAGE_DEFAULT_USE_RANDOM_SEED = 0;
const unsigned int SAPI_QSAGE_DEFAULT_RANDOM_SEED = 0;
const double SAPI_QSAGE_DEFAULT_TIMEOUT = -std::numeric_limits<double>::max();
const int SAPI_QSAGE_DEFAULT_VERBOSE = std::numeric_limits<int>::min();



class LocalInteractionCFindEmbedding : public find_embedding_::LocalInteraction
{
public:
	virtual ~LocalInteractionCFindEmbedding() {}

private:
	virtual void displayOutputImpl(const std::string& msg) const
	{
		printf("%s", msg.c_str());
	}

	virtual bool cancelledImpl() const
	{
		return false;
	}
};


class IsingSolverCQSage : public blackbox::IsingSolver
{
public:
	IsingSolverCQSage(const sapi_Solver* sapiSolverPtr, const sapi_SolverParameters* solverParams) : sapiSolverPtr_(sapiSolverPtr), solverParams_(solverParams)
	{
		if (!sapiSolverPtr_)
			throw blackbox::BlackBoxException("ising solver is invalid");

		const sapi_SolverProperties* solverProperty = sapi_getSolverProperties(sapiSolverPtr_);

		qubits_.resize(solverProperty->quantum_solver->qubits_len);
		std::copy(solverProperty->quantum_solver->qubits, solverProperty->quantum_solver->qubits + solverProperty->quantum_solver->qubits_len, qubits_.begin());

		couplers_.resize(solverProperty->quantum_solver->couplers_len);
		for (size_t i = 0; i < solverProperty->quantum_solver->couplers_len; ++i)
		{
			couplers_[i].first = solverProperty->quantum_solver->couplers[i].q1;
			couplers_[i].second = solverProperty->quantum_solver->couplers[i].q2;
		}

		if (solverProperty->ising_ranges != NULL)
		{
			hMin_ = solverProperty->ising_ranges->h_min;
			hMax_ = solverProperty->ising_ranges->h_max;
		}

		if (solverProperty->ising_ranges != NULL)
		{
			jMin_ = solverProperty->ising_ranges->j_min;
			jMax_ = solverProperty->ising_ranges->j_max;
		}
	}

	virtual ~IsingSolverCQSage()
	{

	}

private:
	virtual void solveIsingImpl(const std::vector<double>& h, const std::map<std::pair<int, int>, double>& j, std::vector<std::vector<int> >& solutions, std::vector<double>& energies, std::vector<int>& numOccurrences) const
	{
    vector<sapi_ProblemEntry> entries;
    entries.reserve(h.size() + j.size());
    for (auto i = 0 * h.size(); i < h.size(); ++i) {
      auto ei = static_cast<int>(i);
      if (h[i] != 0.0) entries.push_back(sapi_ProblemEntry{ei, ei, h[i]});
    }
    BOOST_FOREACH(const auto& e, j) {
      if (e.second != 0.0) entries.push_back(sapi_ProblemEntry{e.first.first, e.first.second, e.second});
    }

		sapi_Problem p;
		p.len = entries.size();
		p.elements = entries.data();

		sapi_IsingResult* isingResult = NULL;
		char err_msg[SAPI_ERROR_MESSAGE_MAX_SIZE];
		if (sapi_solveIsing(sapiSolverPtr_, &p, solverParams_, &isingResult, err_msg) != SAPI_OK)
			throw blackbox::BlackBoxException(std::string("SAPI solver failed: ") + std::string(err_msg));

		solutions.resize(isingResult->num_solutions);
		for (size_t i = 0; i < isingResult->num_solutions; ++i)
		{
			solutions[i].resize(isingResult->solution_len);
			std::copy(isingResult->solutions + i * isingResult->solution_len, isingResult->solutions + (i + 1) * isingResult->solution_len, solutions[i].begin());
		}

		energies.resize(isingResult->num_solutions);
		std::copy(isingResult->energies, isingResult->energies + isingResult->num_solutions, energies.begin());

		if (isingResult->num_occurrences != NULL)
		{
			numOccurrences.resize(isingResult->num_solutions);
			std::copy(isingResult->num_occurrences, isingResult->num_occurrences + isingResult->num_solutions, numOccurrences.begin());
		}

		// deallocate memory for p and ising result
		sapi_freeIsingResult(isingResult);
	}

	const sapi_Solver* sapiSolverPtr_;
	const sapi_SolverParameters* solverParams_;
};

class LocalInteractionCQSage : public blackbox::LocalInteraction
{
public:
	virtual ~LocalInteractionCQSage() {}

private:
	virtual void displayOutputImpl(const std::string& msg) const
	{
		printf("%s", msg.c_str());
	}

	virtual bool cancelledImpl() const
	{
		return false;
	}
};

class BlackBoxObjectiveFunctionC : public blackbox::BlackBoxObjectiveFunction
{
public:
	BlackBoxObjectiveFunctionC(const sapi_QSageObjectiveFunction of, blackbox::LocalInteractionPtr localInteractionPtr, void* extraArg) : BlackBoxObjectiveFunction(localInteractionPtr), of_(of), extraArg_(extraArg)
	{
		if (!of_)
			throw blackbox::BlackBoxException("objective function is invalid");
	}

	virtual std::vector<double> computeObjectValueImpl(const std::vector<std::vector<int> >& states) const
	{
		int numStates = static_cast<int>(states.size());
		int stateLen = static_cast<int>(states[0].size());
		std::vector<double> ret(numStates);
		double* retPtr = &ret[0];
		std::vector<int> statesOneDim;
		for (int i = 0; i < numStates; ++i)
			statesOneDim.insert(statesOneDim.end(), states[i].begin(), states[i].end());

		sapi_Code code = of_(&statesOneDim[0], numStates * stateLen, numStates, extraArg_, retPtr);
		if (code != SAPI_OK)
			throw blackbox::BlackBoxException("The objective_function is incorrect");

		return ret;
	}

	virtual ~BlackBoxObjectiveFunctionC() {}

private:
	const sapi_QSageObjectiveFunction of_;
	void* extraArg_;
};


class LPSolverC : public blackbox::LPSolver
{
public:
	LPSolverC(const sapi_QSageLPSolver lps, blackbox::LocalInteractionPtr localInteractionPtr, void* extraArg) : lps_(lps), LPSolver(localInteractionPtr), extraArg_(extraArg)
	{
		if (!lps_)
			throw blackbox::BlackBoxException("lp_solver is invalid");
	}

	virtual ~LPSolverC() {}

private:
	virtual std::vector<double> solveImpl(const std::vector<double>& f, const std::vector<std::vector<double> >& Aineq, const std::vector<double>& bineq,
		                                  const std::vector<std::vector<double> >& Aeq, const std::vector<double>& beq, const std::vector<double>& lb,
										  const std::vector<double>& ub) const
	{
		// f's length is num_vars
		// Aineq's length is Aineq_size * num_vars
		// bineq's length is Aineq_size
		// Aeq's length is Aeq_size * num_vars
        // beq's length is Aeq_size
		// lb's length is num_vars
		// ub's length is num_vars
		// result's length should be num_vars

		const double* fPtr = NULL;
		if (!f.empty())
			fPtr = &f[0];

		const double* AineqPtr = NULL;
		std::vector<double> AineqOneDim;
		if (!Aineq.empty())
		{
			for (size_t i = 0; i < Aineq.size(); ++i)
				AineqOneDim.insert(AineqOneDim.end(), Aineq[i].begin(), Aineq[i].end());
			AineqPtr = &AineqOneDim[0];
		}

		const double* bineqPtr = NULL;
		if (!bineq.empty())
			bineqPtr = &bineq[0];

		const double* AeqPtr = NULL;
		std::vector<double> AeqOneDim;
		if (!Aeq.empty())
		{
			for (size_t i = 0; i < Aeq.size(); ++i)
				AeqOneDim.insert(AeqOneDim.end(), Aeq[i].begin(), Aeq[i].end());
			AeqPtr = &AeqOneDim[0];
		}

		const double* beqPtr = NULL;
		if (!beq.empty())
			beqPtr = &beq[0];

		const double* lbPtr = NULL;
		if (!lb.empty())
			lbPtr = &lb[0];

		const double* ubPtr = NULL;
		if (!ub.empty())
			ubPtr = &ub[0];

		int numVars = static_cast<int>(f.size());
		int AineqSize = static_cast<int>(Aineq.size());
		int AeqSize = static_cast<int>(Aeq.size());
		std::vector<double> ret(numVars);
		double* retPtr = &ret[0];
		sapi_Code code = lps_(fPtr, AineqPtr, bineqPtr, AeqPtr, beqPtr, lbPtr, ubPtr, numVars, AineqSize, AeqSize, extraArg_, retPtr);
		if (code != SAPI_OK)
			throw blackbox::BlackBoxException("The lp_solver is incorrect");

		return ret;
	}

	const sapi_QSageLPSolver lps_;
	void* extraArg_;
};

} // anonymous namespace


DWAVE_SAPI const sapi_FindEmbeddingParameters SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS =
{
	SAPI_FIND_EMBEDDING_DEFAULT_FAST_EMBEDDING,
	SAPI_FIND_EMBEDDING_DEFAULT_MAX_NO_IMPROVEMENT,
	SAPI_FIND_EMBEDDING_DEFAULT_USE_RANDOM_SEED,
	SAPI_FIND_EMBEDDING_DEFAULT_RANDOM_SEED,
	SAPI_FIND_EMBEDDING_DEFAULT_TIMEOUT,
	SAPI_FIND_EMBEDDING_DEFAULT_TRIES,
	SAPI_FIND_EMBEDDING_DEFAULT_VERBOSE
};

DWAVE_SAPI const sapi_QSageParameters SAPI_QSAGE_DEFAULT_PARAMETERS =
{
	SAPI_QSAGE_DEFAULT_DRAW_SAMPLE,
	SAPI_QSAGE_DEFAULT_EXIT_THRESHOLD_VALUE,
	SAPI_QSAGE_DEFAULT_INITIAL_SOLUTION,
	SAPI_QSAGE_DEFAULT_ISING_QUBO,
	SAPI_QSAGE_DEFAULT_LP_SOLVER,
	SAPI_QSAGE_DEFAULT_LP_SOLVER_EXTRA_ARG,
	SAPI_QSAGE_DEFAULT_MAX_NUM_STATE_EVALUATIONS,
	SAPI_QSAGE_DEFAULT_USE_RANDOM_SEED,
	SAPI_QSAGE_DEFAULT_RANDOM_SEED,
	SAPI_QSAGE_DEFAULT_TIMEOUT,
	SAPI_QSAGE_DEFAULT_VERBOSE
};


DWAVE_SAPI sapi_Code sapi_findEmbedding(const sapi_Problem* S, const sapi_Problem* A, const sapi_FindEmbeddingParameters* find_embedding_params, sapi_Embeddings** embeddings, char* err_msg)
{
	try
	{
		//initialize the std:: maps used to store matrix contents
		std::map<std::pair<int, int>, int> SMap;
		int SSize = 0;

		//check for input problem length
		if(S->len <= 0)
		{
			*embeddings = new sapi_Embeddings;
			(*embeddings)->len = 0;
			(*embeddings)->elements = NULL;
			return SAPI_OK;
		}

		for (size_t i = 0; i < S->len; ++i)
		{
			SMap[std::make_pair(S->elements[i].i, S->elements[i].j)] = 1;
			SSize = std::max(SSize, std::max(S->elements[i].i + 1, S->elements[i].j + 1));
		}

		compressed_matrix::CompressedMatrix<int> SInput(SSize, SSize, SMap);

		std::map<std::pair<int, int>, int> AMap;
		int ASize = 0;
		for (size_t i = 0; i < A->len; ++i)
		{
			AMap[std::make_pair(A->elements[i].i, A->elements[i].j)] = 1;
			ASize = std::max(ASize, std::max(A->elements[i].i + 1, A->elements[i].j + 1));
		}

		compressed_matrix::CompressedMatrix<int> AInput(ASize, ASize, AMap);

		find_embedding_::FindEmbeddingExternalParams feExternalParams;

		if (find_embedding_params->fast_embedding != SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS.fast_embedding)
			feExternalParams.fastEmbedding = find_embedding_params->fast_embedding ? true : false;
		feExternalParams.localInteractionPtr.reset(new LocalInteractionCFindEmbedding());
		if (find_embedding_params->max_no_improvement != SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS.max_no_improvement)
			feExternalParams.maxNoImprovement = find_embedding_params->max_no_improvement;
		if (find_embedding_params->use_random_seed)
			feExternalParams.randomSeed = find_embedding_params->random_seed;
		if (find_embedding_params->timeout != SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS.timeout)
			feExternalParams.timeout = find_embedding_params->timeout;
		if (find_embedding_params->tries != SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS.tries)
			feExternalParams.tries = find_embedding_params->tries;
		if (find_embedding_params->verbose != SAPI_FIND_EMBEDDING_DEFAULT_PARAMETERS.verbose)
			feExternalParams.verbose = find_embedding_params->verbose;

		std::vector<std::vector<int> > embs = find_embedding_::findEmbedding(SInput, AInput, feExternalParams);
		// check out put for invalid no embedded solution
		if(embs.size() <= 0)
		{
			if (S->len > 0)
			{
				writeErrorMessage(err_msg, "Failed to find embedding.");
				//return error code which indicates no embedding solution found
				return SAPI_ERR_NO_EMBEDDING_FOUND;
			}
		}

		*embeddings = new sapi_Embeddings;
		(*embeddings)->len = ASize;
		(*embeddings)->elements = new int[(*embeddings)->len];
		memset((*embeddings)->elements, -1, (*embeddings)->len * sizeof(int));
		for (size_t i = 0; i < embs.size(); ++i)
		for (size_t j = 0; j < embs[i].size(); ++j)
			(*embeddings)->elements[embs[i][j]] = i;

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), err_msg);
	}
}


DWAVE_SAPI sapi_Code sapi_solveQSage(const sapi_QSageObjFunc* obj_func, const sapi_Solver* solver, const sapi_SolverParameters* solver_params, const sapi_QSageParameters* qsage_params, sapi_QSageResult** qsage_result, char* err_msg)
{
	try
	{
		blackbox::LocalInteractionPtr localInteractionPtr(new LocalInteractionCQSage());
		blackbox::BlackBoxObjectiveFunctionPtr blackBoxObjectiveFunctionPtr(new BlackBoxObjectiveFunctionC(obj_func->objective_function, localInteractionPtr, obj_func->objective_function_extra_arg));

		blackbox::IsingSolverPtr isingSolverPtr(new IsingSolverCQSage(solver, solver_params));

		blackbox::BlackBoxExternalParams blackBoxExternalParams;

		if (qsage_params->draw_sample != SAPI_QSAGE_DEFAULT_PARAMETERS.draw_sample)
			blackBoxExternalParams.drawSample = qsage_params->draw_sample ? true : false;

		if (qsage_params->exit_threshold_value != SAPI_QSAGE_DEFAULT_PARAMETERS.exit_threshold_value)
			blackBoxExternalParams.exitThresholdValue = qsage_params->exit_threshold_value;

		if (qsage_params->initial_solution != SAPI_QSAGE_DEFAULT_PARAMETERS.initial_solution)
		{
			for (int i = 0; i < obj_func->num_vars; ++i)
				blackBoxExternalParams.initialSolution.push_back(qsage_params->initial_solution[i]);
		}

		if (qsage_params->ising_qubo != SAPI_QSAGE_DEFAULT_PARAMETERS.ising_qubo)
		{
			if (qsage_params->ising_qubo == SAPI_PROBLEM_TYPE_QUBO)
				blackBoxExternalParams.isingQubo = blackbox::QUBO;
		}

		blackBoxExternalParams.localInteractionPtr = localInteractionPtr;

		if (qsage_params->lp_solver != SAPI_QSAGE_DEFAULT_PARAMETERS.lp_solver)
			blackBoxExternalParams.lpSolverPtr.reset(new LPSolverC(qsage_params->lp_solver, localInteractionPtr, qsage_params->lp_solver_extra_arg));

		if (qsage_params->max_num_state_evaluations != SAPI_QSAGE_DEFAULT_PARAMETERS.max_num_state_evaluations)
			blackBoxExternalParams.maxNumStateEvaluations = qsage_params->max_num_state_evaluations;

		if (qsage_params->use_random_seed)
			blackBoxExternalParams.randomSeed = qsage_params->random_seed;

		if (qsage_params->timeout != SAPI_QSAGE_DEFAULT_PARAMETERS.timeout)
			blackBoxExternalParams.timeout = qsage_params->timeout;

		if (qsage_params->verbose != SAPI_QSAGE_DEFAULT_PARAMETERS.verbose)
			blackBoxExternalParams.verbose = qsage_params->verbose;

		*qsage_result = dwave_sapi::construct_qsage_result(blackbox::solveBlackBox(blackBoxObjectiveFunctionPtr, obj_func->num_vars, isingSolverPtr, blackBoxExternalParams));

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), err_msg);
	}
}


DWAVE_SAPI sapi_Code sapi_getChimeraAdjacency(int M, int N, int L, sapi_Problem** A)
{
	try
	{
		*A = new sapi_Problem;
		std::vector<sapi_ProblemEntry> ret;
		sapi_ProblemEntry tmp;
		tmp.value = 1.0;

		// vertical edges:
		for (int j = 0; j < N; ++j)
		{
			int start = L * 2 * j;
			for (int i = 0; i < M - 1; ++i)
			{
				for (int t = 0; t < L; ++t)
				{
					int fm = start + t;
					int to = start + t + N * L * 2;
					tmp.i = fm;
					tmp.j = to;
					ret.push_back(tmp);
					tmp.i = to;
					tmp.j = fm;
					ret.push_back(tmp);
				}
				start += N * L * 2;
			}
		}

		// horizontal edges:
		for (int i = 0; i < M; ++i)
		{
			int start = L * (2 * N * i + 1);
			for (int j = 0; j < N - 1; ++j)
			{
				for (int t = 0; t < L; ++t)
				{
					int fm = start + t;
					int to = start + t + L * 2;
					tmp.i = fm;
					tmp.j = to;
					ret.push_back(tmp);
					tmp.i = to;
					tmp.j = fm;
					ret.push_back(tmp);
				}
				start += L * 2;
			}
		}

		// inside-cell edges:
		for (int i = 0; i < M; ++i)
		{
			for (int j = 0; j < N; ++j)
			{
				int add = (i * N + j) * L * 2;
				for (int t = 0; t < L; ++t)
					for (int u = L; u < 2 * L; ++u)
					{
						tmp.i = t + add;
						tmp.j = u + add;
						ret.push_back(tmp);
						tmp.i = u + add;
						tmp.j = t + add;
						ret.push_back(tmp);
					}
			}
		}

		(*A)->len = static_cast<int>(ret.size());
		(*A)->elements = new sapi_ProblemEntry[(*A)->len];
		std::copy(ret.begin(), ret.end(), (*A)->elements);

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), 0);
	}
}


DWAVE_SAPI sapi_Code sapi_getHardwareAdjacency(const sapi_Solver* solver, sapi_Problem** A)
{
	try
	{
		const sapi_SolverProperties* solver_property = solver->properties();

		(*A) = new sapi_Problem;
		(*A)->len = 2 * solver_property->quantum_solver->couplers_len;
		(*A)->elements = new sapi_ProblemEntry[(*A)->len];
		for (size_t i = 0; i < solver_property->quantum_solver->couplers_len; ++i)
		{
			(*A)->elements[2 * i].i = solver_property->quantum_solver->couplers[i].q1;
			(*A)->elements[2 * i].j = solver_property->quantum_solver->couplers[i].q2;
			(*A)->elements[2 * i].value = 1.0;
			(*A)->elements[2 * i + 1].i = solver_property->quantum_solver->couplers[i].q2;
			(*A)->elements[2 * i + 1].j = solver_property->quantum_solver->couplers[i].q1;
			(*A)->elements[2 * i + 1].value = 1.0;
		}

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), 0);
	}
}


DWAVE_SAPI sapi_Code sapi_reduceDegree(const sapi_Terms* terms, sapi_Terms** new_terms, sapi_VariablesRep** variables_rep, char* err_msg)
{
	try
	{
		std::vector<std::set<int> > t(terms->len);
		for (size_t i = 0; i < terms->len; ++i)
		{
			for (size_t j = 0; j < terms->elements[i].len; ++j)
				t[i].insert(terms->elements[i].terms[j]);
		}

		dwave_sapi::TermsResult termsResult = dwave_sapi::reduce_degree(t);

		*new_terms = new sapi_Terms;
		(*new_terms)->len = static_cast<int>(termsResult.newTerms.size());
		(*new_terms)->elements = new sapi_TermsEntry[termsResult.newTerms.size()];
		for (size_t i = 0; i < termsResult.newTerms.size(); ++i)
		{
			(*new_terms)->elements[i].len = static_cast<int>(termsResult.newTerms[i].size());
			(*new_terms)->elements[i].terms = new int[termsResult.newTerms[i].size()];
			int j = 0;
			for (std::set<int>::const_iterator it = termsResult.newTerms[i].begin(), end = termsResult.newTerms[i].end(); it != end; ++it)
				(*new_terms)->elements[i].terms[j++] = *it;
		}

		*variables_rep = new sapi_VariablesRep;
		(*variables_rep)->len = static_cast<int>(termsResult.mapping.size());
		(*variables_rep)->elements = new sapi_VariablesRepEntry[termsResult.mapping.size()];
		for (size_t i = 0; i < termsResult.mapping.size(); ++i)
		{
			(*variables_rep)->elements[i].variable = termsResult.mapping[i][0];
			(*variables_rep)->elements[i].rep[0] = termsResult.mapping[i][1];
			(*variables_rep)->elements[i].rep[1] = termsResult.mapping[i][2];
		}

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), err_msg);
	}
}


DWAVE_SAPI sapi_Code sapi_makeQuadratic(const double* f, size_t f_len, const double* penalty_weight, sapi_Terms** terms, sapi_VariablesRep** variables_rep, sapi_Problem** Q, char* err_msg)
{
	try
	{
		std::vector<double> fVector(f_len);
		for (size_t i = 0; i < f_len; ++i)
			fVector[i] = f[i];

		dwave_sapi::TermsResult termsResult = dwave_sapi::make_quadratic(fVector, penalty_weight);

		*terms = new sapi_Terms;
		(*terms)->len = static_cast<int>(termsResult.newTerms.size());
		(*terms)->elements = new sapi_TermsEntry[termsResult.newTerms.size()];
		for (size_t i = 0; i < termsResult.newTerms.size(); ++i)
		{
			(*terms)->elements[i].len = static_cast<int>(termsResult.newTerms[i].size());
			(*terms)->elements[i].terms = new int[termsResult.newTerms[i].size()];
			int j = 0;
			for (std::set<int>::const_iterator it = termsResult.newTerms[i].begin(), end = termsResult.newTerms[i].end(); it != end; ++it)
				(*terms)->elements[i].terms[j++] = *it;
		}

		*variables_rep = new sapi_VariablesRep;
		(*variables_rep)->len = static_cast<int>(termsResult.mapping.size());
		(*variables_rep)->elements = new sapi_VariablesRepEntry[termsResult.mapping.size()];
		for (size_t i = 0; i < termsResult.mapping.size(); ++i)
		{
			(*variables_rep)->elements[i].variable = termsResult.mapping[i][0];
			(*variables_rep)->elements[i].rep[0] = termsResult.mapping[i][1];
			(*variables_rep)->elements[i].rep[1] = termsResult.mapping[i][2];
		}

		*Q = new sapi_Problem;
		(*Q)->len = static_cast<int>(termsResult.Q.size());
		(*Q)->elements = new sapi_ProblemEntry[termsResult.Q.size()];
		int j = 0;
		for (std::map<std::pair<int, int>, double>::const_iterator it = termsResult.Q.begin(), end = termsResult.Q.end(); it != end; ++it)
		{
			(*Q)->elements[j].i = it->first.first;
			(*Q)->elements[j].j = it->first.second;
			(*Q)->elements[j++].value = it->second;
		}

		return SAPI_OK;
	}
	catch (...)
	{
		return handleException(current_exception(), err_msg);
	}
}

