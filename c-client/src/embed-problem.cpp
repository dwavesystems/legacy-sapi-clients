//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cmath>
#include <cstring>
#include <algorithm>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include "dwave_sapi.h"
#include "internal.hpp"

using std::size_t;
using std::current_exception;
using std::fill;
using std::make_pair;
using std::map;
using std::numeric_limits;
using std::pair;
using std::set;
using std::unique_ptr;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using sapi::InvalidParameterException;
using sapi::EdgeSet;
using sapi::Embeddings;
using sapi::IsingProblem;
using sapi::SparseMatrix;
using sapi::toIsingProblem;
using sapi::decodeEmbeddings;
using sapi::handleException;

namespace {

struct EmbeddedProblem {
  std::vector<double> h;
  SparseMatrix j;
  EdgeSet jc;
};

const sapi_IsingRangeProperties defaultIsingRange = { -1.0, 1.0, -1.0, 1.0 };

bool comparePair(const std::pair<std::pair<int, int>, double>& a, const std::pair<std::pair<int, int>, double>& b)
{
  return a.second < b.second;
}

bool areConnected(const vector<int>& emb, const EdgeSet& adj, size_t adjSize)
{
  std::vector<char> visited(adjSize, 0);
  std::queue<int> q;
  int curr = emb[0];
  q.push(curr);
  visited[curr] = 1;
  size_t cnt = 1;
  while (!q.empty())
  {
    curr = q.front(); q.pop();
    for (size_t i = 0; i < emb.size(); ++i)
    {
      if (!visited[emb[i]] && adj.find(std::make_pair(curr, emb[i])) != adj.end())
      {
        visited[emb[i]] = 1;
        ++cnt;
        q.push(emb[i]);
      }
    }
  }

  return cnt == emb.size();
}

void validateEmbVars(const Embeddings& embeddings, const EdgeSet& adj, size_t adjSize) {
  for (size_t i = 0; i < embeddings.size(); ++i) {
    if (embeddings[i].empty()) {
      std::stringstream ss;
      ss << "logical variable " << i << " has empty embedding";
      throw InvalidParameterException(ss.str());
    }

    BOOST_FOREACH( auto v, embeddings[i] ) {
      if (adj.find(make_pair(v, v)) == adj.end()) {
        std::stringstream ss;
        ss << "invalid vertex in logical variable " << i << ": " << v;
        throw InvalidParameterException(ss.str());
      }
    }

    if (!areConnected(embeddings[i], adj, adjSize)) {
      std::stringstream ss;
      ss << "embedding of logical variable " << i << " does not induce a connected graph";
      throw InvalidParameterException(ss.str());
    }
  }
}

// j has no zero entries, and j is upper-triangular
void validateEmbEdges(const Embeddings& embeddings, const SparseMatrix& j, const EdgeSet& adj) {
  for (auto jit = j.begin(), end = j.end(); jit != end; ++jit) {
    int q1 = jit->first.first;
    int q2 = jit->first.second;
    auto connected = false;

    for (size_t k = 0; k < embeddings[q1].size() && !connected; ++k) {
      for (size_t t = 0; t < embeddings[q2].size() && !connected; ++t) {
        connected = adj.find(std::make_pair(embeddings[q1][k], embeddings[q2][t])) != adj.end();
      }
    }

    if (!connected) {
      std::stringstream ss;
      ss << "logical variables " << q1 << " and " << q2 << " are not adjacent after embedding";
      throw InvalidParameterException(ss.str());
    }
  }
}


Embeddings cleanEmbedding(
    const vector<double>& h,
    const SparseMatrix& j,
    Embeddings embeddings,
    const EdgeSet& adj) {

  auto used = unordered_set<int>{};

  BOOST_FOREACH( const auto& k, j ) {
    if (k.second != 0) {
      used.insert(k.first.first);
      used.insert(k.first.second);
    }
  }

  for(size_t i = 0; i < h.size(); ++i) {
    if (h[i]) {
      used.insert(i);
    }
  }

  for(size_t i = 0; i < embeddings.size(); ++i) {
    if (used.find(i) == used.end()) {
      embeddings[i].resize(1);
    }
  }

  auto interchainVars = unordered_set<int>{};
  BOOST_FOREACH( const auto& e, j ) {
    if (e.second != 0.0) { // additional check for aggressive pruning
      BOOST_FOREACH( auto u, embeddings[e.first.first] ) {
        BOOST_FOREACH( auto v, embeddings[e.first.second] ) {
          if (adj.find(make_pair(u, v)) != adj.end()) {
            interchainVars.insert(u);
            interchainVars.insert(v);
          }
        }
      }
    }
  }

  auto newEmbeddings = Embeddings{};
  newEmbeddings.reserve(embeddings.size());
  BOOST_FOREACH( const auto& emb, embeddings ) {
    auto leaves = unordered_set<int>{};
    auto embNbrs = unordered_map<int, unordered_set<int>>{};
    BOOST_FOREACH( auto u, emb ) {
      if (interchainVars.find(u) == interchainVars.end()) {
        auto nbrs = unordered_set<int>{};
        BOOST_FOREACH( auto v, emb ) {
          if (v != u && adj.find(make_pair(u, v)) != adj.end()) nbrs.insert(v);
        }
        if (nbrs.size() == 1) leaves.insert(u);
        embNbrs[u] = std::move(nbrs);
      }
    }

    auto pruned = unordered_set<int>{};
    while (!leaves.empty()) {
      auto leafIt = leaves.begin();
      auto leaf = *leafIt;
      leaves.erase(leafIt);
      const auto& leafNbrs = embNbrs.at(leaf);
      if (leafNbrs.empty()) continue; // this prevents isolated paths from disappearing
      pruned.insert(leaf);
      auto nbr = *leafNbrs.begin();
      auto nbrNbrsIt = embNbrs.find(nbr);
      if (nbrNbrsIt != embNbrs.end()) {
        nbrNbrsIt->second.erase(leaf);
        if (nbrNbrsIt->second.size() == 1) leaves.insert(nbr);
      }
    }

    auto newEmb = vector<int>{};
    BOOST_FOREACH( auto v, emb ) {
      if (pruned.find(v) == pruned.end()) newEmb.push_back(v);
    }
    newEmbeddings.push_back(std::move(newEmb));
  }

  return newEmbeddings;
}

// suppose that embeddings.size() == h.size()
Embeddings smearEmbedding(
    const IsingProblem& isingProblem,
    const Embeddings& embeddings,
    const EdgeSet& adj,
    size_t adjSize,
    const sapi_IsingRangeProperties& ranges) {

  if (isingProblem.j.empty()) return embeddings;
  if (ranges.h_min >= 0.0) throw InvalidParameterException("h range must include negative numbers");
  if (ranges.h_max <= 0.0) throw InvalidParameterException("h range must include positive numbers");
  if (ranges.j_min >= 0.0) throw InvalidParameterException("J range must include negative numbers");
  if (ranges.j_max <= 0.0) throw InvalidParameterException("J range must include positive numbers");

  auto jScale = numeric_limits<double>::infinity();
  BOOST_FOREACH( const auto je, isingProblem.j ) {
    auto mult = 0.0;
    BOOST_FOREACH( auto u, embeddings[je.first.first]) {
      BOOST_FOREACH( auto v, embeddings[je.first.second]) {
        if (adj.find(make_pair(u, v)) != adj.end()) ++mult;
      }
    }
    jScale = std::min(jScale, (je.second > 0 ? ranges.j_max : ranges.j_min) * mult / je.second);
  }

  auto hScales = vector<pair<double, size_t>>{};
  for (auto i = 0u; i < isingProblem.h.size(); ++i) {
    if (isingProblem.h[i] > 0) {
      hScales.push_back(make_pair(ranges.h_max * embeddings[i].size() / isingProblem.h[0], i));
    } else if (isingProblem.h[i] < 0) {
      hScales.push_back(make_pair(ranges.h_min * embeddings[i].size() / isingProblem.h[0], i));
    }
  }
  std::sort(hScales.begin(), hScales.end());

  auto used = unordered_set<int>{};
  BOOST_FOREACH( const auto& emb, embeddings ) {
    BOOST_FOREACH( auto v, emb ) {
      used.insert(v);
    }
  }

  auto adjv = unordered_map<int, unordered_set<int>>{};
  BOOST_FOREACH( const auto& edge, adj ) {
    if (edge.first < edge.second) {
      if (used.find(edge.first) == used.end()) adjv[edge.second].insert(edge.first);
      if (used.find(edge.second) == used.end()) adjv[edge.first].insert(edge.second);
    }
  }

  auto newEmbeddings = Embeddings(embeddings.size());
  BOOST_FOREACH( const auto& hsi, hScales ) {
    if (hsi.first >= jScale) break;
    auto targetSize = jScale * isingProblem.h[hsi.second] / (hsi.second > 0 ? ranges.h_max : ranges.h_min);
    auto emb = embeddings[hsi.second];
    auto avail = unordered_set<int>{};
    while (emb.size() < targetSize) {
      if (avail.empty()) {
        BOOST_FOREACH( auto u, emb ) {
          BOOST_FOREACH( auto v, adjv[u] ) {
            if (used.find(v) == used.end()) avail.insert(v);
          }
        }
        if (avail.empty()) break;
      } else {
        auto availIt = avail.begin();
        emb.push_back(*availIt);
        used.insert(*availIt);
        avail.erase(availIt);
      }
    }
    newEmbeddings[hsi.second] = std::move(emb);
  }

  for (auto i = 0u; i < newEmbeddings.size(); ++i) {
    if (newEmbeddings[i].empty()) newEmbeddings[i] = embeddings[i];
  }
  return newEmbeddings;
}


EmbeddedProblem embedProblem(
    const IsingProblem problem,
    const Embeddings& embeddings,
    const EdgeSet& adj,
    size_t adjSize) {

  auto h0 = vector<double>(adjSize, 0.0);
  auto jc = EdgeSet{};
  for (auto i = 0u; i < embeddings.size(); ++i) {
    const auto& embi = embeddings[i];
    auto embHi = problem.h[i] / embi.size();
    BOOST_FOREACH( auto j, embi ) {
      h0[j] = embHi;
    }

    for (auto j = 0u; j < embi.size(); ++j) {
      for (auto k = j + 1; k < embi.size(); ++k) {
        auto eu = embi[j];
        auto ev = embi[k];
        if (eu > ev) std::swap(eu, ev);
        auto p = make_pair(eu, ev);
        if (adj.find(p) != adj.end()) jc.insert(make_pair(eu, ev));
      }
    }
  }

  auto j0 = SparseMatrix{};
  auto jPairs = vector<pair<int, int>>{};
  BOOST_FOREACH( const auto& jv, problem.j ) {
    jPairs.clear();
    BOOST_FOREACH( auto embi, embeddings[jv.first.first] ) {
      BOOST_FOREACH( auto embj, embeddings[jv.first.second] ) {
        auto p = make_pair(embi, embj);
        if (adj.find(p) != adj.end()) {
          if (embi > embj) std::swap(p.first, p.second);
          jPairs.push_back(p);
        }
      }
    }

    auto embJ = jv.second / jPairs.size();
    BOOST_FOREACH( const auto& p, jPairs ) {
      j0[p] = embJ;
    }
  }

  return EmbeddedProblem{std::move(h0), std::move(j0), std::move(jc)};
}

sapi_EmbedProblemResult* convertProblemResult(const EmbeddedProblem& problem, const Embeddings& embeddings) {
  size_t numH = 0;
  BOOST_FOREACH( auto hi, problem.h ) {
    if (hi != 0.0) ++numH;
  }

  auto problemSize = numH + problem.j.size();
  auto probEntries = unique_ptr<sapi_ProblemEntry>{new sapi_ProblemEntry[problemSize]};
  auto jcEntries = unique_ptr<sapi_ProblemEntry>{new sapi_ProblemEntry[problem.jc.size()]};
  auto embEntries = unique_ptr<int>{new int[problem.h.size()]};
  fill(embEntries.get(), embEntries.get() + problem.h.size(), -1);

  auto result = unique_ptr<sapi_EmbedProblemResult>{new sapi_EmbedProblemResult};
  result->problem.len = problemSize;
  result->problem.elements = probEntries.get();
  result->jc.len = problem.jc.size();
  result->jc.elements = jcEntries.get();
  result->embeddings.len = problem.h.size();
  result->embeddings.elements = embEntries.get();

  auto eptr = probEntries.get();
  for (size_t i = 0; i < problem.h.size(); ++i) {
    if (problem.h[i] != 0.0) {
      eptr->i = eptr->j = i;
      eptr->value = problem.h[i];
      ++eptr;
    }
  }
  BOOST_FOREACH( const auto& e, problem.j ) {
    eptr->i = e.first.first;
    eptr->j = e.first.second;
    eptr->value = e.second;
    ++eptr;
  }

  eptr = jcEntries.get();
  BOOST_FOREACH( const auto& e, problem.jc ) {
    eptr->i = e.first;
    eptr->j = e.second;
    eptr->value = -1.0;
    ++eptr;
  }

  for (size_t i = 0; i < embeddings.size(); ++i) {
    BOOST_FOREACH( auto v, embeddings[i] ) {
      embEntries.get()[v] = i;
    }
  }

  probEntries.release();
  jcEntries.release();
  embEntries.release();
  return result.release();
}

} // namespace {anonymous}

namespace sapi {

IsingProblem toIsingProblem(const sapi_Problem* sp) {
  IsingProblem ip;
  int maxJi = -1;
  for (size_t i = 0; i < sp->len; ++i) {
    const auto& elem = sp->elements[i];
    if (elem.i < 0 || elem.j < 0 || elem.i == numeric_limits<int>::max() || elem.j == numeric_limits<int>::max()) {
      throw InvalidParameterException("invalid variable index");
    }
    if (elem.i == elem.j) {
      if (ip.h.size() < static_cast<size_t>(elem.i) + 1) ip.h.resize(static_cast<size_t>(elem.i) + 1);
      ip.h[elem.i] += elem.value;
    } else {
      auto p = make_pair(elem.i, elem.j);
      if (p.first > p.second) std::swap(p.first, p.second);
      ip.j[p] += elem.value;
      maxJi = std::max(maxJi, p.second);
    }
  }
  ++maxJi;
  if (ip.h.size() < static_cast<size_t>(maxJi)) ip.h.resize(static_cast<size_t>(maxJi));

  for (auto jit = ip.j.begin(), end = ip.j.end(); jit != end; ) {
    if (jit->second == 0.0) {
      jit = ip.j.erase(jit);
    } else {
      ++jit;
    }
  }
  return ip;
}

Embeddings decodeEmbeddings(const sapi_Embeddings* cemb) {
  auto maxItem = std::max_element(cemb->elements, cemb->elements + cemb->len);
  if (maxItem == cemb->elements + cemb->len) return Embeddings{};

  auto maxEmb = *maxItem;
  if (maxEmb == numeric_limits<int>::max()) throw InvalidParameterException("invalid logical variable index");
  maxEmb = std::max(0, maxEmb + 1);

  auto emb = Embeddings(maxEmb);
  for (size_t i = 0; i < cemb->len; ++i) {
    if (cemb->elements[i] >= 0) emb[cemb->elements[i]].push_back(i);
  }

  return emb;
}

} // namespace sapi

sapi_Code sapi_embedProblem(
    const sapi_Problem* problem,
    const sapi_Embeddings* embeddings,
    const sapi_Problem* adj,
    int clean,
    int smear,
    const sapi_IsingRangeProperties* ranges,
    sapi_EmbedProblemResult** result,
    char* err_msg) {


  try
  {
    auto isingProblem = toIsingProblem(problem);

    auto embeddingsVec = decodeEmbeddings(embeddings);
    if (isingProblem.h.size() < embeddingsVec.size()) {
      isingProblem.h.resize(embeddingsVec.size());
    } else if (isingProblem.h.size() > embeddingsVec.size()) {
      throw InvalidParameterException("problem has more variables than embeddings");
    }

    // A_new is the original A with diagonal
    auto adjSet = EdgeSet{};
    size_t adjSize = 0;
    for (size_t i = 0; i < adj->len; ++i) {
      auto q1 = adj->elements[i].i;
      auto q2 = adj->elements[i].j;
      if (q1 < 0 || q2 < 0 || q1 == numeric_limits<int>::max() || q2 == numeric_limits<int>::max()) {
        throw InvalidParameterException("invalid adjacency matrix index");
      }

      adjSize = std::max(adjSize, static_cast<size_t>(std::max(q1, q2)) + 1);
      adjSet.insert(make_pair(q1, q1));
      adjSet.insert(make_pair(q2, q2));
      adjSet.insert(make_pair(q1, q2));
      adjSet.insert(make_pair(q2, q1));
    }

    validateEmbVars(embeddingsVec, adjSet, adjSize);
    validateEmbEdges(embeddingsVec, isingProblem.j, adjSet);

    if (clean) embeddingsVec = cleanEmbedding(isingProblem.h, isingProblem.j, std::move(embeddingsVec), adjSet);

    if (smear) {
      if (!ranges) ranges = &defaultIsingRange;
      embeddingsVec = smearEmbedding(isingProblem, embeddingsVec, adjSet, adjSize, *ranges);
    }

    auto embeddedProblem = embedProblem(isingProblem, embeddingsVec, adjSet, adjSize);

    *result = convertProblemResult(embeddedProblem, embeddingsVec);
    return SAPI_OK;

  } catch (...) {
    return handleException(current_exception(), err_msg);
  }
}


DWAVE_SAPI void sapi_freeEmbedProblemResult(sapi_EmbedProblemResult* result) {
  if (result) {
    delete[] result->problem.elements;
    delete[] result->jc.elements;
    delete[] result->embeddings.elements;
    delete result;
  }
}
