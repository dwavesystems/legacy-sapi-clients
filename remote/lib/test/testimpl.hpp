//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef LIB_TESTIMPL_HPP_INCLUDED
#define LIB_TESTIMPL_HPP_INCLUDED

#include <string>

#include <problem-manager.hpp>
#include <http-service.hpp>

namespace testimpl {

sapiremote::ProblemManagerPtr createProblemManager(
    std::string url, std::string token, sapiremote::http::Proxy proxy);

} // namespace testimpl

#endif
