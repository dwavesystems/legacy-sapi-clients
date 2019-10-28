//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <utility>

#include <problem-manager.hpp>
#include <http-service.hpp>

#include <testimpl.hpp>
#include "../sapiremote-mex.hpp"

using sapiremote::ProblemManagerPtr;

ProblemManagerPtr makeProblemManager(ConnectionInfo conninfo) {
  return testimpl::createProblemManager(
      std::move(conninfo.url), std::move(conninfo.token), std::move(conninfo.proxy));
}

void exitFn() {}
