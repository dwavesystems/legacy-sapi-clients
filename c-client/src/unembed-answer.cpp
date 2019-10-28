//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstddef>
#include <cmath>
#include <chrono>
#include <exception>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/random.hpp>

#include "dwave_sapi.h"
#include "internal.hpp"

using std::abs;
using std::size_t;
using std::current_exception;
using std::make_pair;
using std::pair;
using std::vector;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

using boost::random::mt19937;
using boost::random::uniform_int_distribution;
using boost::random::uniform_real_distribution;

using sapi::InvalidParameterException;
using sapi::Embeddings;
using sapi::IsingProblem;
using sapi::decodeEmbeddings;
using sapi::toIsingProblem;
using sapi::handleException;

namespace {

void unembedMinimizeEnergy(
    const int* solutions,
    size_t solutionLen,
    size_t numSolutions,
    const Embeddings& embeddings,
    const IsingProblem& problem,
    int* newSolutions,
    size_t* numNewSolutions) {

  for (size_t si = 0; si < numSolutions; ++si) {
    vector<int> broken;
    for (size_t ei = 0; ei < embeddings.size(); ++ei) {
      if (embeddings[ei].empty()) continue;
      auto val = solutions[embeddings[ei][0]];
      bool ok = true;
      BOOST_FOREACH( const auto& v, embeddings[ei] ) {
        if (solutions[v] != val) {
          ok = false;
          break;
        }
      }
      if (ok) {
        newSolutions[ei] = val;
      } else {
        newSolutions[ei] = 0;
        broken.push_back(ei);
      }
    }

    vector<pair<double, int>> energies;
    energies.reserve(broken.size());
    BOOST_FOREACH( auto i, broken ) {
      double e = static_cast<int>(problem.h.size()) > i ? problem.h[i] : 0.0;
      for (auto j = 0; j < static_cast<int>(embeddings.size()); ++j) {
        auto p = i < j ? make_pair(i, j) : make_pair(j, i);
        auto jit = problem.j.find(p);
        if (jit != problem.j.end()) e += jit->second * newSolutions[j];
      }
      energies.push_back(make_pair(e, i));
    }

    while (!energies.empty()) {
      auto maxAbsE = -1.0;
      size_t maxI = 0;
      for (size_t i = 0; i < energies.size(); ++i) {
        if (abs(energies[i].first) > maxAbsE) {
          maxAbsE = abs(energies[i].first);
          maxI = i;
        }
      }

      auto maxE = energies[maxI];
      auto nsi = maxE.first > 0.0 ? -1 : 1;
      newSolutions[maxE.second] = nsi;
      energies[maxI] = energies.back();
      energies.pop_back();

      BOOST_FOREACH( auto& ee, energies ) {
        auto p = make_pair(maxE.second, ee.second);
        if (p.first > p.second) std::swap(p.first, p.second);
        auto jit = problem.j.find(p);
        if (jit != problem.j.end()) ee.first += jit->second * nsi;
      }
    }

    solutions += solutionLen;
    newSolutions += embeddings.size();
  }

  *numNewSolutions = numSolutions;
}

void unembedVote(
    const int* solutions,
    size_t solutionLen,
    size_t numSolutions,
    const Embeddings& embeddings,
    int* newSolutions,
    size_t* numNewSolutions) {

  auto rngEng = mt19937(high_resolution_clock::now().time_since_epoch().count());
  auto rng = uniform_int_distribution<int>(0, 1);

  for (size_t si = 0; si < numSolutions; ++si) {
    for (size_t ei = 0; ei < embeddings.size(); ++ei) {
      size_t numOnes = 0;
      BOOST_FOREACH( const auto& v, embeddings[ei] ) {
        if (solutions[v] == 1) ++numOnes;
      }

      auto nonOnes = embeddings[ei].size() - numOnes;
      if (numOnes > nonOnes) {
        newSolutions[ei] = 1;
      } else if (numOnes < nonOnes) {
        newSolutions[ei] = -1;
      } else {
        newSolutions[ei] = 2 * rng(rngEng) - 1;
      }
    }

    solutions += solutionLen;
    newSolutions += embeddings.size();
  }

  *numNewSolutions = numSolutions;
}

void unembedDiscard(
    const int* solutions,
    size_t solutionLen,
    size_t numSolutions,
    const Embeddings& embeddings,
    int* newSolutions,
    size_t* numNewSolutions) {

  *numNewSolutions = 0;
  for (size_t si = 0; si < numSolutions; ++si) {
    bool ok = true;

    for (size_t i = 0; ok && i < embeddings.size(); ++i) {
      if (embeddings[i].empty()) continue;
      auto val = solutions[embeddings[i][0]];
      BOOST_FOREACH( const auto& v, embeddings[i] ) {
        if (solutions[v] != val) {
          ok = false;
          break;
        }
      }
      newSolutions[i] = val;
    }

    solutions += solutionLen;
    if (ok) {
      newSolutions += embeddings.size();
      ++*numNewSolutions;
    }
  }
}


void unembedWeightedRandom(
    const int* solutions,
    size_t solutionLen,
    size_t numSolutions,
    const Embeddings& embeddings,
    int* newSolutions,
    size_t* numNewSolutions) {

  auto rngEng = mt19937(high_resolution_clock::now().time_since_epoch().count());
  auto rng = uniform_real_distribution<double>(0, 1);

  for (size_t si = 0; si < numSolutions; ++si) {
    for (size_t ei = 0; ei < embeddings.size(); ++ei) {
      size_t numOnes = 0;
      BOOST_FOREACH( const auto& v, embeddings[ei] ) {
        if (solutions[v] == 1) ++numOnes;
      }
      newSolutions[ei] = rng(rngEng) < static_cast<double>(numOnes) / embeddings[ei].size() ? 1 : -1;
    }

    solutions += solutionLen;
    newSolutions += embeddings.size();
  }

  *numNewSolutions = numSolutions;
}

} // namespace {anonymous}

DWAVE_SAPI sapi_Code sapi_unembedAnswer(
    const int* solutions,
    size_t solutionLen,
    size_t numSolutions,
    const sapi_Embeddings* embeddings,
    sapi_BrokenChains broken_chains,
    const sapi_Problem* problem,
    int* newSolutions,
    size_t* numNewSolutions,
    char* err_msg) {

  try {
    auto embeddingsVec = decodeEmbeddings(embeddings);

    switch (broken_chains) {
      case SAPI_BROKEN_CHAINS_MINIMIZE_ENERGY:
        {
          if (!problem) throw InvalidParameterException("problem required for minimize energy unembedding");
          auto isingProblem = toIsingProblem(problem);
          if (isingProblem.h.size() < embeddingsVec.size()) {
            isingProblem.h.resize(embeddingsVec.size());
          } else if (isingProblem.h.size() > embeddingsVec.size()) {
            throw InvalidParameterException("problem is larger than embeddings");
          }
          unembedMinimizeEnergy(solutions, solutionLen, numSolutions, embeddingsVec, toIsingProblem(problem),
            newSolutions, numNewSolutions);
        }
        break;
      case SAPI_BROKEN_CHAINS_VOTE:
        unembedVote(solutions, solutionLen, numSolutions, embeddingsVec, newSolutions, numNewSolutions);
        break;
      case SAPI_BROKEN_CHAINS_DISCARD:
        unembedDiscard(solutions, solutionLen, numSolutions, embeddingsVec, newSolutions, numNewSolutions);
        break;
      case SAPI_BROKEN_CHAINS_WEIGHTED_RANDOM:
        unembedWeightedRandom(solutions, solutionLen, numSolutions, embeddingsVec, newSolutions, numNewSolutions);
        break;
      default:
        throw InvalidParameterException("invalid broken_chains value");
    }

    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}
