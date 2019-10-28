//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef INTERNAL_HPP_INCLUDED
#define INTERNAL_HPP_INCLUDED

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include <problem-manager.hpp>

#include "dwave_sapi.h"

namespace sapi {

class UnsupportedAnswerFormatException : public std::runtime_error {
public:
  UnsupportedAnswerFormatException(const std::string& msg) : std::runtime_error(msg) {}
};

class InvalidParameterException : public std::runtime_error {
public:
  InvalidParameterException(const std::string& msg) : std::runtime_error(msg) {}
};

class NotInitializedException : public std::runtime_error {
public:
  NotInitializedException() : std::runtime_error("global state not initialized") {}
};

void writeErrorMessage(char* destination, const char* source);
sapi_Code handleException(std::exception_ptr e, char* errMsg);

typedef std::pair<int, int> Edge;
typedef std::unordered_map<Edge, double, boost::hash<Edge>> SparseMatrix;
typedef std::unordered_set<Edge, boost::hash<Edge>> EdgeSet;
typedef std::vector<std::vector<int>> Embeddings;

struct IsingProblem {
  std::vector<double> h;
  SparseMatrix j;
};

IsingProblem toIsingProblem(const sapi_Problem* sp);
Embeddings decodeEmbeddings(const sapi_Embeddings* cemb);

typedef std::unique_ptr<sapi_SubmittedProblem> SubmittedProblemPtr;
typedef std::shared_ptr<sapi_Solver> SolverPtr; // shared because g++ 4.4 hates move-only map elements
typedef std::unique_ptr<sapi_Connection> ConnectionPtr;

typedef std::unordered_map<std::string, sapi::SolverPtr> SolverMap;

struct IsingResultDeleter {
  void operator()(sapi_IsingResult* p) { sapi_freeIsingResult(p); }
};

typedef std::unique_ptr<sapi_IsingResult, IsingResultDeleter> IsingResultPtr;

sapiremote::ProblemManagerPtr makeProblemManager(const char* url, const char* token, const char* proxy);

} // namespace sapi

#endif
