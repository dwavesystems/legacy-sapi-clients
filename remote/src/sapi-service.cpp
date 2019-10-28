//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cctype>
#include <cstdio>
#include <exception>
#include <iterator>
#include <memory>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>

#include <exceptions.hpp>
#include <http-service.hpp>
#include <sapi-service.hpp>
#include <json.hpp>

#include "user-agent.hpp"

using std::isalnum;
using std::exception_ptr;
using std::ostringstream;
using std::shared_ptr;
using std::string;
using std::make_shared;
using std::make_pair;
using std::next;
using std::current_exception;
using std::vector;

using sapiremote::http::HttpServicePtr;
using sapiremote::http::HttpHeaders;
using sapiremote::http::HttpCallback;
using sapiremote::http::Proxy;
using sapiremote::SapiService;
using sapiremote::SolversSapiCallbackPtr;
using sapiremote::StatusSapiCallbackPtr;
using sapiremote::CancelSapiCallbackPtr;
using sapiremote::FetchAnswerSapiCallbackPtr;
using sapiremote::RemoteProblemInfo;
using sapiremote::SolverInfo;
using sapiremote::CommunicationException;
using sapiremote::AuthenticationException;
using sapiremote::NoAnswerException;
using sapiremote::SolveException;
using sapiremote::ProblemCancelledException;
using sapiremote::Problem;

namespace remotestatuses = sapiremote::remotestatuses;

namespace {

namespace headers {
const char* authToken = "X-Auth-Token";
const char* contentType = "Content-Type";
const char* userAgent = "User-Agent";
} // namespace {anonymous}::headers

const char* applicationJson = "application/json";

namespace paths {
const char* remoteSolvers = "solvers/remote/";
const char* problems = "problems/";
} // namespace {anonymous}::paths

namespace solverkeys {
const char* solverId = "id";
const char* properties = "properties";
}

namespace submitkeys {
const char* type = "type";
const char* solver = "solver";
const char* data = "data";
const char* params = "params";
} // namespace {anonymous}::submitkeys

namespace problemkeys {
const char* problemType = "type";
const char* problemId = "id";
const char* status = "status";
const char* submittedOn = "submitted_on";
const char* solvedOn = "solved_on";
const char* answer = "answer";
const char* errorMessage = "error_message";
}

namespace status {
const char* pending = "PENDING";
const char* inProgress = "IN_PROGRESS";
const char* completed = "COMPLETED";
const char* failed = "FAILED";
const char* canceled = "CANCELED";
const char* cancelled = "CANCELLED";
}

struct KeyException {
  string key;
  KeyException(string key0) : key(key0) {}
};

class HttpStatusException : public CommunicationException {
public:
  HttpStatusException(int status, const std::string& url) :
    CommunicationException(string("HTTP status code " + std::to_string(static_cast<long long>(status))), url) {}
};

class MissingKeyException : public CommunicationException {
public:
  MissingKeyException(const KeyException& ke, const string& url) :
    CommunicationException("missing key: " + ke.key, url) {}
};

class JsonFormatException : public CommunicationException {
public:
  JsonFormatException(const string& url) : CommunicationException("JSON format error", url) {}
};

class MissingStatusException : public CommunicationException {
public:
  MissingStatusException(const string& id, const string& url) :
    CommunicationException("no status provided for problem ID " + id, url) {}
};

class UnknownStatusException : public CommunicationException {
public:
  UnknownStatusException(const string& status, const string& url) :
    CommunicationException("unknown problem status: " + status, url) {}
};

class StatusSizeException : public CommunicationException {
public:
  StatusSizeException(const string& url) :
    CommunicationException("incorrect number of problem statuses provided", url) {}
};


void checkHttpResponse(int code, int expected, const string& url) {
  if (code != expected) {
    if (code == sapiremote::http::statusCodes::UNAUTHORIZED) {
      throw AuthenticationException();
    } else {
      throw HttpStatusException(code, url);
    }
  }
}

remotestatuses::Type problemStatus(const string& ss) {
  if (ss == status::pending) {
    return remotestatuses::PENDING;
  } else if (ss == status::inProgress) {
    return remotestatuses::IN_PROGRESS;
  } else if (ss == status::completed) {
    return remotestatuses::COMPLETED;
  } else if (ss == status::failed) {
    return remotestatuses::FAILED;
  } else if (ss == status::canceled || ss == status::cancelled) {
    return remotestatuses::CANCELED;
  } else {
    return remotestatuses::UNKNOWN;
  }
}

const json::Value& getKey(const json::Object& obj, const string& key) {
  try {
    return obj.at(key);
  } catch (std::out_of_range&) {
    throw KeyException(key);
  }
}

json::Value& getKey(json::Object& obj, const string& key) {
  try {
    return obj.at(key);
  } catch (std::out_of_range&) {
    throw KeyException(key);
  }
}

string getErrorMessage(json::Object& obj) {
  auto iter = obj.find(problemkeys::errorMessage);
  if (iter != obj.end()) {
    return std::move(iter->second.getString());
  } else {
    return "(no error message provided)";
  }
}

HttpHeaders makeGetHeaders(std::string token);
HttpHeaders makePostHeaders(const HttpHeaders& getHeaders);

string fixToken(string);
string fixBaseUrl(string baseUrl);
string percentEscape(const string& s);

class SapiServiceImpl : public SapiService {
private:
  HttpServicePtr httpService_;
  const string baseUrl_;
  const string problemsUrl_;
  const string token_;
  const Proxy proxy_;
  HttpHeaders getHeaders_;
  HttpHeaders postHeaders_;

  virtual void fetchSolversImpl(SolversSapiCallbackPtr callback);
  virtual void submitProblemsImpl(vector<Problem>& problems, StatusSapiCallbackPtr callback);
  virtual void multiProblemStatusImpl(const vector<string>& ids, StatusSapiCallbackPtr callback);
  virtual void fetchAnswerImpl(const std::string& id, FetchAnswerSapiCallbackPtr callback);
  virtual void cancelProblemsImpl(const std::vector<std::string>& ids, CancelSapiCallbackPtr callback);

  template<typename T>
  std::string url(const T& path) { return baseUrl_ + path; }

public:
  SapiServiceImpl(HttpServicePtr httpService, string baseUrl, string token, Proxy proxy);
};

class SolversHttpCallback : public HttpCallback {
private:
  string url_;
  SolversSapiCallbackPtr callback_;
  virtual void completeImpl(int statusCode, shared_ptr<string> data);
  virtual void errorImpl(exception_ptr e) { callback_->error(e); }
public:
  SolversHttpCallback(string url, SolversSapiCallbackPtr callback) : url_(std::move(url)), callback_(callback) {}
};

class StatusHttpCallback : public HttpCallback {
private:
  string url_;
  StatusSapiCallbackPtr callback_;
  const size_t expectedNumProblems_;
  const int expectedHttpStatus_;

  virtual void completeImpl(int statusCode, shared_ptr<string> data);
  virtual void errorImpl(exception_ptr e) { callback_->error(e); }

public:
  StatusHttpCallback(string url, StatusSapiCallbackPtr callback, size_t expectedNumProblems, int expectedHttpStatus) :
    url_(std::move(url)),
    callback_(callback),
    expectedNumProblems_(expectedNumProblems),
    expectedHttpStatus_(expectedHttpStatus) {}
};

class FetchAnswerHttpCallback : public HttpCallback {
private:
  string url_;
  FetchAnswerSapiCallbackPtr callback_;
  virtual void completeImpl(int statusCode, shared_ptr<string> data);
  virtual void errorImpl(exception_ptr e) { callback_->error(e); }
public:
  FetchAnswerHttpCallback(string url, FetchAnswerSapiCallbackPtr callback) :
    url_(std::move(url)),
    callback_(callback) {}
};

class CancelHttpCallback : public HttpCallback {
private:
  CancelSapiCallbackPtr callback_;
  virtual void completeImpl(int, shared_ptr<string>) { callback_->complete(); }
  virtual void errorImpl(exception_ptr e) { callback_->error(e); }
public:
  CancelHttpCallback(CancelSapiCallbackPtr callback) : callback_(callback) {}
};

SapiServiceImpl::SapiServiceImpl(
    HttpServicePtr httpService,
    string baseUrl,
    string token,
    Proxy proxy) :
        httpService_(httpService),
        baseUrl_(fixBaseUrl(std::move(baseUrl))),
        problemsUrl_(baseUrl_ + paths::problems),
        proxy_(std::move(proxy)),
        getHeaders_(makeGetHeaders(fixToken(std::move(token)))),
        postHeaders_(makePostHeaders(getHeaders_)) {}

void SapiServiceImpl::fetchSolversImpl(SolversSapiCallbackPtr callback) {
  auto u = url(paths::remoteSolvers);
  auto httpCallback = make_shared<SolversHttpCallback>(u, callback);
  httpService_->asyncGet(u, getHeaders_, proxy_, httpCallback);
}

void SapiServiceImpl::submitProblemsImpl(vector<Problem>& problems, StatusSapiCallbackPtr callback) {
  json::Array request;
  request.reserve(problems.size());

  BOOST_FOREACH( auto& p, problems ) {
    json::Object problemJson;
    problemJson[submitkeys::solver] = std::move(p.solver());
    problemJson[submitkeys::type] = std::move(p.type());
    problemJson[submitkeys::data] = std::move(p.data());
    problemJson[submitkeys::params] = std::move(p.params());
    request.push_back(std::move(problemJson));
  }

  auto httpCallback = make_shared<StatusHttpCallback>(
      problemsUrl_, callback, problems.size(), sapiremote::http::statusCodes::OK);
  httpService_->asyncPost(problemsUrl_, postHeaders_, jsonToString(request), proxy_, httpCallback);
}

void SapiServiceImpl::multiProblemStatusImpl(const vector<string>& ids, StatusSapiCallbackPtr callback) {

  if (ids.empty()) {
    callback->complete(vector<RemoteProblemInfo>());
    return;
  }

  try {
    string query = "?id=";
    query.append(percentEscape(ids.front()));
    BOOST_FOREACH( const std::string& id, make_pair(next(ids.begin()), ids.end()) ) {
      query.append(1, ',').append(percentEscape(id));
    }

    auto u = problemsUrl_ + query;
    auto httpCallback = make_shared<StatusHttpCallback>(u, callback, ids.size(), sapiremote::http::statusCodes::OK);
    httpService_->asyncGet(u, getHeaders_, proxy_, httpCallback);
  } catch (...) {
    callback->error(current_exception());
  }
}

void SapiServiceImpl::fetchAnswerImpl(const string& id, FetchAnswerSapiCallbackPtr callback) {
  try {
    auto u = problemsUrl_ + id + "/";
    auto httpCallback = make_shared<FetchAnswerHttpCallback>(u, callback);
    httpService_->asyncGet(u, getHeaders_, proxy_, httpCallback);
  } catch (...) {
    callback->error(current_exception());
  }
}

void SapiServiceImpl::cancelProblemsImpl(const std::vector<std::string>& ids, CancelSapiCallbackPtr callback) {
  try {
    auto idsJson = json::Array(ids.begin(), ids.end());
    httpService_->asyncDelete(problemsUrl_, getHeaders_, json::jsonToString(idsJson), proxy_,
      make_shared<CancelHttpCallback>(callback));
  } catch (...) {
    callback->error(current_exception());
  }
}


void SolversHttpCallback::completeImpl(int statusCode, shared_ptr<string> data) {
  try {
    checkHttpResponse(statusCode, sapiremote::http::statusCodes::OK, url_);

    json::Value jsonData = json::stringToJson(*data);
    auto& solversJsonArray = jsonData.getArray();

    vector<SolverInfo> solvers;
    solvers.reserve(solversJsonArray.size());
    BOOST_FOREACH( auto& v, solversJsonArray ) {
      auto& solverJsonObj = v.getObject();
      SolverInfo si;
      si.id = std::move(getKey(solverJsonObj, solverkeys::solverId).getString());
      si.properties = std::move(getKey(solverJsonObj, solverkeys::properties).getObject());
      solvers.push_back(std::move(si));
    }

    callback_->complete(std::move(solvers));

  } catch (json::Exception&) {
    throw JsonFormatException(url_);
  } catch (KeyException& e) {
    throw MissingKeyException(e, url_);
  }
}


void StatusHttpCallback::completeImpl(int statusCode, shared_ptr<string> data) {
  try {
    checkHttpResponse(statusCode, expectedHttpStatus_, url_);

    auto dataJson = json::stringToJson(*data);
    auto& dataArray = dataJson.getArray();
    if (dataArray.size() != expectedNumProblems_) throw StatusSizeException(url_);

    vector<RemoteProblemInfo> problemInfos;
    problemInfos.reserve(dataArray.size());

    BOOST_FOREACH( auto& v, dataArray ) {
      RemoteProblemInfo pi;
      if (v.isNull()) {
        pi.status = remotestatuses::FAILED;

      } else {
        auto& vObj = v.getObject();
        pi.id = getKey(vObj, problemkeys::problemId).getString();
        pi.type = getKey(vObj, problemkeys::problemType).getString();
        auto statusString = getKey(vObj, problemkeys::status).getString();
        pi.status = problemStatus(statusString);

        auto iter = vObj.find(problemkeys::submittedOn);
        if (iter != vObj.end() && iter->second.isString()) pi.submittedOn = iter->second.getString();
        iter = vObj.find(problemkeys::solvedOn);
        if (iter != vObj.end() && iter->second.isString()) pi.solvedOn = iter->second.getString();

        if (pi.status == remotestatuses::FAILED) pi.errorMessage = getErrorMessage(vObj);
        if (pi.status == remotestatuses::UNKNOWN) throw UnknownStatusException(statusString, url_);
      }
      problemInfos.push_back(std::move(pi));
    }
    callback_->complete(std::move(problemInfos));

  } catch (json::Exception&) {
    throw JsonFormatException(url_);
  } catch (KeyException& e) {
    throw MissingKeyException(e, url_);
  }
}

void FetchAnswerHttpCallback::completeImpl(int statusCode, shared_ptr<string> data) {
  try {
    checkHttpResponse(statusCode, sapiremote::http::statusCodes::OK, url_);

    auto dataJson = json::stringToJson(*data);
    auto& dataObj = dataJson.getObject();

    auto ps = getKey(dataObj, problemkeys::status).getString();
    switch (problemStatus(ps)) {
      case remotestatuses::COMPLETED:
        callback_->complete(std::move(getKey(dataObj, problemkeys::problemType).getString()),
            std::move(getKey(dataObj, problemkeys::answer)));
        break;

      case remotestatuses::PENDING:
      case remotestatuses::IN_PROGRESS:
        throw NoAnswerException();

      case remotestatuses::FAILED:
        throw SolveException(getErrorMessage(dataObj));

      case remotestatuses::CANCELED:
        throw ProblemCancelledException();

      default:
        throw UnknownStatusException(ps, url_);
    }

  } catch (json::Exception&) {
    throw JsonFormatException(url_);
  } catch (KeyException& e) {
    throw MissingKeyException(e, url_);
  }
}

HttpHeaders makeGetHeaders(std::string token) {
  HttpHeaders h;
  h[headers::authToken] = token;
  h[headers::userAgent] = sapiremote::userAgent;
  return h;
}

HttpHeaders makePostHeaders(const HttpHeaders& getHeaders) {
  HttpHeaders h = getHeaders;
  h[headers::contentType] = applicationJson;
  return h;
}


string fixToken(string token) {
  // strip leading and trailing whitespace
  // validate remaining characters are in range [32, 126]
  static const auto minValid = 33;
  static const auto maxValid = 126;
  static const auto spaces = " \t\n\r\v\f";

  auto i1 = token.find_last_not_of(spaces);
  if (i1 != string::npos) token.erase(i1 + 1);

  auto i0 = token.find_first_not_of(spaces);
  token.erase(0, i0);

  BOOST_FOREACH( auto ch, token ) {
    if (ch < minValid || ch > maxValid) throw AuthenticationException();
  }
  return token;
}

string fixBaseUrl(string baseUrl) {
  size_t i1 = baseUrl.find_first_of('?');
  if (i1 != string::npos) --i1;
  size_t i2 = baseUrl.find_last_not_of('/', i1);
  if (i2 != string::npos) {
    baseUrl.resize(i2 + 1);
  } else if (i1 != string::npos) {
    baseUrl.resize(i1 + 1);
  }
  baseUrl.append("/");
  return baseUrl;
}

string percentEscape(const string& s) {
  static const char hex[] = "0123456789abcdef";
  const int bufSize = 4;
  char buf[bufSize]; // %xx + \0
  string escaped;
  escaped.reserve(s.size());

  BOOST_FOREACH( char c, s ) {
    switch (c) {
      case '-':
      case '.':
      case '_':
      case '~':
        escaped.append(1, c);
        break;
      default:
        if (isalnum(c)) {
          escaped.append(1, c);
        } else {
          escaped.append(1, '%');
          escaped.append(1, hex[(c >> 4) & 0xf]);
          escaped.append(1, hex[c & 0xf]);
          escaped.append(buf);
        }
        break;
    }
  }
  return escaped;
}

} // namespace {anonymous}

namespace sapiremote {

SapiServicePtr makeSapiService(
    http::HttpServicePtr httpService,
    std::string baseUrl,
    std::string token,
    http::Proxy proxy) {

  return make_shared<SapiServiceImpl>(httpService, std::move(baseUrl), std::move(token), std::move(proxy));
}

} // namespace sapiremote
