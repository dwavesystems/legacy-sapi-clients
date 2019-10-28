//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include "conversions.h"

#include <algorithm>

#include "coding.hpp"
#include <internal.hpp>

using sapi::InvalidParameterException;
using sapi::UnsupportedAnswerFormatException;

namespace
{

bool comparePairBelongsTo(const std::pair<std::pair<int, int>, std::set<int> >& a, const std::pair<std::pair<int, int>, std::set<int> >& b)
{
	return a.second.size() < b.second.size();
}

}

namespace dwave_sapi
{

sapi_QSageResult* construct_qsage_result(const blackbox::BlackBoxResult& blackBoxResult)
{
	sapi_QSageResult* ret = new sapi_QSageResult;

	ret->len = static_cast<int>(blackBoxResult.bestSolution.size());
	ret->best_solution = new int[blackBoxResult.bestSolution.size()];
	std::copy(blackBoxResult.bestSolution.begin(), blackBoxResult.bestSolution.end(), ret->best_solution);
	ret->best_energy = blackBoxResult.bestEnergy;

	ret->info.stat.num_state_evaluations = blackBoxResult.info.numStateEvaluations;
	ret->info.stat.num_obj_func_calls = blackBoxResult.info.numObjFuncCalls;
	ret->info.stat.num_solver_calls = blackBoxResult.info.numSolverCalls;
	ret->info.stat.num_lp_solver_calls = blackBoxResult.info.numLPSolverCalls;
	ret->info.state_evaluations_time = blackBoxResult.info.stateEvaluationsTime;
	ret->info.solver_calls_time = blackBoxResult.info.solverCallsTime;
	ret->info.lp_solver_calls_time = blackBoxResult.info.lpSolverCallsTime;
	ret->info.total_time = blackBoxResult.info.totalTime;
	ret->info.progress.elements = new sapi_QSageProgressEntry[blackBoxResult.info.progressTable.size()];
	ret->info.progress.len = static_cast<int>(blackBoxResult.info.progressTable.size());

	for (int i = 0; i < blackBoxResult.info.progressTable.size(); ++i)
	{
		ret->info.progress.elements[i].stat.num_state_evaluations = blackBoxResult.info.progressTable[i].first[0];
		ret->info.progress.elements[i].stat.num_obj_func_calls = blackBoxResult.info.progressTable[i].first[1];
		ret->info.progress.elements[i].stat.num_solver_calls = blackBoxResult.info.progressTable[i].first[2];
		ret->info.progress.elements[i].stat.num_lp_solver_calls = blackBoxResult.info.progressTable[i].first[3];
		ret->info.progress.elements[i].time = blackBoxResult.info.progressTable[i].second.first;
		ret->info.progress.elements[i].energy = blackBoxResult.info.progressTable[i].second.second;
	}

	return ret;
}


TermsResult reduce_degree(const std::vector<std::set<int> >& terms)
{
	for (int i = 0; i < terms.size(); ++i)
	{
		for (std::set<int>::const_iterator it = terms[i].begin(), end = terms[i].end(); it != end; ++it)
		{
			if (*it < 0)
				throw InvalidParameterException("terms should contain non-negative integers");
		}
	}

	TermsResult ret;
	int maxVar = 0;
	std::vector<std::set<int> > tmpTerms = terms;

	std::map<std::pair<int, int>, std::set<int> > pairBelongsTo;
	for (int i = 0; i < tmpTerms.size(); ++i)
	{
		for (std::set<int>::const_iterator it = tmpTerms[i].begin(), end = tmpTerms[i].end(); it != end; ++it)
		{
			if (maxVar < *it)
				maxVar = *it;
		}

		if (tmpTerms[i].size() > 2)
		{
			for (std::set<int>::const_iterator it1 = tmpTerms[i].begin(), end = tmpTerms[i].end(); it1 != end; ++it1)
			{
				std::set<int>::const_iterator it2 = it1;
				++it2;
				for (; it2 != end; ++it2)
					pairBelongsTo[std::make_pair(*it1, *it2)].insert(i);
			}
		}
	}

	while (!pairBelongsTo.empty())
	{
		std::map<std::pair<int, int>, std::set<int> >::const_iterator it = std::max_element(pairBelongsTo.begin(), pairBelongsTo.end(), comparePairBelongsTo);
		++maxVar;
		std::vector<int> tmp;
		tmp.push_back(maxVar);
		tmp.push_back(it->first.first);
		tmp.push_back(it->first.second);
		ret.mapping.push_back(tmp);

		std::vector<int> affected(it->second.begin(), it->second.end());

		for (int i = 0; i < affected.size(); ++i)
		{
			int k = affected[i];
			for (std::set<int>::const_iterator it1 = tmpTerms[k].begin(), end1 = tmpTerms[k].end(); it1 != end1; ++it1)
			{
				std::set<int>::const_iterator it2 = it1;
				++it2;
				for (; it2 != end1; ++it2)
				{
					pairBelongsTo[std::make_pair(*it1, *it2)].erase(k);
					if (pairBelongsTo[std::make_pair(*it1, *it2)].empty())
						pairBelongsTo.erase(std::make_pair(*it1, *it2));
				}
			}

			tmpTerms[k].erase(ret.mapping.back()[1]);
			tmpTerms[k].erase(ret.mapping.back()[2]);
			tmpTerms[k].insert(maxVar);

			if (tmpTerms[k].size() > 2)
			{
				for (std::set<int>::const_iterator it1 = tmpTerms[k].begin(), end = tmpTerms[k].end(); it1 != end; ++it1)
				{
					std::set<int>::const_iterator it2 = it1;
					++it2;
					for (; it2 != end; ++it2)
						pairBelongsTo[std::make_pair(*it1, *it2)].insert(k);
				}
			}
		}
	}

	ret.newTerms = tmpTerms;

	return ret;
}

TermsResult make_quadratic(const std::vector<double>& f, const double* penaltyWeight)
{
	int n = static_cast<int>(f.size());

	if (!(n > 0 && (n & (n - 1)) == 0))
		throw InvalidParameterException("f's length is not power of 2");

	if (f[0] != 0.0)
		throw InvalidParameterException("sapi_makeQuadratic expects the first element of f, the constant to be zero");

	int numVars = 0;
	while ((1 << numVars) != n)
		++numVars;

	std::vector<std::vector<int> > invW(n, std::vector<int>(n, 0));

	invW[0][0] = 1;

	if (numVars >= 1)
	{
		invW[1][0] = -1;
		invW[1][1] = 1;
	}

	for (int i = 1; i < numVars; ++i)
	{
		int tmp = (1 << i);
		for (int j = 0; j < tmp; ++j)
			for (int k = 0; k < tmp; ++k)
			{
				// -1 to kron (from MATLAB) [[1, 0], [-1, 1]]
				invW[j + tmp][k] = -invW[j][k];
				invW[j + tmp][k + tmp] = invW[j][k];
			}
	}

	std::vector<double> c(n, 0.0);
	for (int i = 0; i < n; ++i)
		for (int j = 0; j < n; ++j)
			c[i] += invW[i][j] * f[j];

	std::vector<std::set<int> > terms;
	std::vector<int> nonZeroIndex;
	for (int i = 0; i < n; ++i)
	{
		if (abs(c[i]) > 1e-10)
		{
			nonZeroIndex.push_back(i);
			std::set<int> tmp;
			for (int j = 0; j < numVars; ++j)
			{
				if (i & (1 << j))
					tmp.insert(j);
			}
			terms.push_back(tmp);
		}
	}

	TermsResult ret = reduce_degree(terms);

	int newNumVars = numVars + static_cast<int>(ret.mapping.size());

	std::vector<double> r(newNumVars, 0.0);

	for (int i = 0; i < ret.newTerms.size(); ++i)
	{
		if (ret.newTerms[i].size() == 1)
			r[(*ret.newTerms[i].begin())] += c[nonZeroIndex[i]];
		else if (ret.newTerms[i].size() == 2)
		{
			std::set<int>::const_iterator it = ret.newTerms[i].begin();
			int index1 = *it;
			++it;
			int index2 = *it;
			ret.Q[std::make_pair(index1, index2)] += c[nonZeroIndex[i]] / 2.0;
			ret.Q[std::make_pair(index2, index1)] += c[nonZeroIndex[i]] / 2.0;
		}
		else
			throw InvalidParameterException("f should only have pairwise interactions");
	}

	double pw;
	if (penaltyWeight == NULL)
	{
		pw = 0.0;
		for (std::map<std::pair<int, int>, double>::const_iterator it = ret.Q.begin(), end = ret.Q.end(); it != end; ++it)
			pw = std::max(pw, std::abs(it->second));

		pw *= 10;
	}
	else
		pw = *penaltyWeight;

	if (newNumVars > numVars)
	{
		double QAnd[3][3] =
		{
			{0.0, -pw, -pw},
		    {-pw, 0.0, pw / 2.0},
			{-pw, pw / 2.0, 0.0}
		};

		double rAnd[3] = {3.0 * pw, 0.0, 0.0};

		for (int k = 0; k < ret.mapping.size(); ++k)
			for (int i = 0; i < ret.mapping[k].size(); ++i)
			{
				for (int j = 0; j < ret.mapping[k].size(); ++j)
					ret.Q[std::make_pair(ret.mapping[k][i], ret.mapping[k][j])] += QAnd[i][j];
				r[ret.mapping[k][i]] += rAnd[i];
			}
	}

	std::map<std::pair<int, int>, double> zeroBasedQ;
	for (std::map<std::pair<int, int>, double>::const_iterator it = ret.Q.begin(), end = ret.Q.end(); it != end; ++it)
	{
		if (it->second != 0.0)
			zeroBasedQ[std::make_pair(it->first.first, it->first.second)] += it->second;
	}

	for (int i = 0; i < r.size(); ++i)
	{
		if (r[i] != 0.0)
			zeroBasedQ[std::make_pair(i, i)] += r[i];
	}

	ret.Q = zeroBasedQ;

	return ret;
}

} // namespace dwave_sapi
