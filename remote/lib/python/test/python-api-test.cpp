//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <string>
#include <utility>

#include <problem-manager.hpp>
#include <http-service.hpp>

#include <testimpl.hpp>
#include "python-api.hpp"

using std::string;

using sapiremote::ProblemManagerPtr;
using sapiremote::http::Proxy;

ProblemManagerPtr createProblemManager(string& url, string& token, Proxy& proxy) {
  return testimpl::createProblemManager(std::move(url), std::move(token), std::move(proxy));
}

