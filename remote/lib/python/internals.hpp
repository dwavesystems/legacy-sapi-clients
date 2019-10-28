//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef PYTHON_INTERNALS_HPP_INCLUDED
#define PYTHON_INTERNALS_HPP_INCLUDED

#include <map>
#include <string>

#include <problem-manager.hpp>
#include <http-service.hpp>

class Solver;

sapiremote::ProblemManagerPtr createProblemManager(
    std::string& url, std::string& token, sapiremote::http::Proxy& proxy);

std::map<std::string, Solver> fetchSolvers(sapiremote::ProblemManagerPtr problemManager);

#endif
