//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef DWAVE_SAPI_CONVERSIONS_HPP_INCLUDED
#define DWAVE_SAPI_CONVERSIONS_HPP_INCLUDED

#include "dwave_sapi.h"
#include "blackbox.hpp"

#include <vector>
#include <map>
#include <utility>
#include <set>

namespace dwave_sapi
{

sapi_QSageResult* construct_qsage_result(const blackbox::BlackBoxResult& blackBoxResult);

struct TermsResult
{
	std::map<std::pair<int, int>, double> Q;
	std::vector<std::set<int> > newTerms;
	std::vector<std::vector<int> > mapping;
};

TermsResult reduce_degree(const std::vector<std::set<int> >& terms);

TermsResult make_quadratic(const std::vector<double>& f, const double* penaltyWeight = NULL);

} // namespace dwave_sapi

#endif // DWAVE_SAPI_CONVERSIONS_HPP_INCLUDED
