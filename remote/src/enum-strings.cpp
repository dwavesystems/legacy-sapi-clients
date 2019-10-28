//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <string>

#include <types.hpp>

using std::string;

namespace sapiremote {

template<> string toString<remotestatuses::Type>(remotestatuses::Type status) {
  switch (status) {
    case remotestatuses::PENDING:
      return "PENDING";
    case remotestatuses::IN_PROGRESS:
      return "IN_PROGRESS";
    case remotestatuses::COMPLETED:
      return "COMPLETED";
    case remotestatuses::FAILED:
      return "FAILED";
    case remotestatuses::CANCELED:
      return "CANCELED";
    default:
      return "UNKNOWN";
  }
}

template<> string toString<submittedstates::Type>(submittedstates::Type state) {
  switch (state) {
    case submittedstates::SUBMITTING:
      return "SUBMITTING";
    case submittedstates::SUBMITTED:
      return "SUBMITTED";
    case submittedstates::DONE:
      return "DONE";
    case submittedstates::RETRYING:
      return "RETRYING";
    case submittedstates::FAILED:
      return "FAILED";
    default:
      return "UNKNOWN";
  }
}

template<> string toString<errortypes::Type>(errortypes::Type type) {
  switch (type) {
    case errortypes::NETWORK:
      return "NETWORK";
    case errortypes::PROTOCOL:
      return "PROTOCOL";
    case errortypes::AUTH:
      return "AUTH";
    case errortypes::SOLVE:
      return "SOLVE";
    case errortypes::MEMORY:
      return "MEMORY";
    default:
      return "INTERNAL";
  }
}

} // namespace sapiremote
