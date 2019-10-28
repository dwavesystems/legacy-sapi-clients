//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <mex.h>

#include <http-service.hpp>
#include "sapiremote-mex.hpp"

using std::make_shared;
using std::ostringstream;
using std::string;
using std::unique_ptr;
using std::unordered_map;

using sapiremote::http::Proxy;

namespace {
namespace connfields {
auto url = "url";
auto token = "token";
auto proxy = "proxy";
} // namespace {anonymous}::connfields

ConnectionInfo extractConnectionInfo(const mxArray* connArray) {
  ConnectionInfo connInfo;

  auto urlField = mxGetField(connArray, 0, connfields::url);
  auto tokenField = mxGetField(connArray, 0, connfields::token);
  if (!urlField || !tokenField) failBadHandle();

  auto url = unique_ptr<char, MxFreeDeleter>(mxArrayToString(urlField));
  auto token = unique_ptr<char, MxFreeDeleter>(mxArrayToString(tokenField));
  if (!url || !token) failBadHandle();

  auto proxyField = mxGetField(connArray, 0, connfields::proxy);
  if (proxyField && mxIsChar(proxyField)) {
    auto proxy = unique_ptr<char, MxFreeDeleter>(mxArrayToString(proxyField));
    if (proxy) connInfo.proxy = Proxy(proxy.get());
  } else if (proxyField && !mxIsEmpty(proxyField)) {
    failBadHandle();
  }

  connInfo.url = url.get();
  connInfo.token = token.get();
  return connInfo;
}

string connectionId(const ConnectionInfo& connInfo) {
  ostringstream str;
  str << connInfo.url.size() << connInfo.url << connInfo.token.size() << connInfo.token;
  if (connInfo.proxy.enabled()) str << connInfo.proxy.url().size() << connInfo.proxy.url();
  return str.str();
}

} // namespace {anonymous}

ConnectionPtr getConnection(const mxArray* connArray) {
  static unordered_map<string, ConnectionPtr> connections;

  auto connInfo = extractConnectionInfo(connArray);
  auto connId = connectionId(connInfo);
  auto iter = connections.find(connId);
  if (iter != connections.end()) {
    return iter->second;
  } else {
    auto conn = make_shared<Connection>(makeProblemManager(std::move(connInfo)));
    connections[connId] = conn;
    return conn;
  }
}

