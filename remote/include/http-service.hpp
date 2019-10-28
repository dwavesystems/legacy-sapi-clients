//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef HTTP_SERVICE_HPP_INCLUDED
#define HTTP_SERVICE_HPP_INCLUDED

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace sapiremote {
namespace http {

namespace statusCodes {
enum Type {
  OK = 200,
  UNAUTHORIZED = 401,
  REQUEST_URI_TOO_LONG = 414,
};
} // namespace sapiremote::http::statusCodes

class Proxy {
private:
  std::string url_;
  bool enabled_;
public:
  Proxy() : url_(), enabled_(false) {}
  Proxy(std::string url) : url_(url), enabled_(true) {}
  Proxy(const Proxy& other) : url_(other.url_), enabled_(other.enabled_) {}
  Proxy(Proxy&& other) : url_(std::move(other.url_)), enabled_(other.enabled_) {}
  Proxy& operator=(const Proxy& other) { url_ = other.url_; enabled_ = other.enabled_; return *this; }
  Proxy& operator=(Proxy&& other) { url_.swap(other.url_); enabled_ = other.enabled_; return *this; }
  const std::string& url() const { return url_; }
  bool enabled() const { return enabled_; }
};

class HttpCallback {
private:
  virtual void completeImpl(int statusCode, std::shared_ptr<std::string> data) = 0;
  virtual void errorImpl(std::exception_ptr e) = 0;
public:
  virtual ~HttpCallback() {}
  void complete(int statusCode, std::shared_ptr<std::string> data) {
    try {
      completeImpl(statusCode, data);
    } catch (...) {
      error(std::current_exception());
    }
  }

  void error(std::exception_ptr e) {
    try {
      errorImpl(e);
    } catch (...) {
      // ruh roh
    }
  }
};
typedef std::shared_ptr<HttpCallback> HttpCallbackPtr;

typedef std::map<std::string, std::string> HttpHeaders;

class HttpService {
  virtual void asyncGetImpl(
      const std::string& url,
      const HttpHeaders& headers,
      const Proxy& proxy,
      HttpCallbackPtr callback) = 0;
  virtual void asyncPostImpl(
      const std::string& url,
      const HttpHeaders& headers,
      std::string& data,
      const Proxy& proxy,
      HttpCallbackPtr callback) = 0;
  virtual void asyncDeleteImpl(
      const std::string& url,
      const HttpHeaders& headers,
      std::string& data,
      const Proxy& proxy,
      HttpCallbackPtr callback) = 0;
  virtual void shutdownImpl() = 0;
public:
  virtual ~HttpService() {}

  void asyncGet(
      const std::string& url,
      const HttpHeaders& headers,
      const Proxy& proxy,
      HttpCallbackPtr callback) { asyncGetImpl(url, headers, proxy, callback); }
  void asyncPost(
      const std::string& url,
      const HttpHeaders& headers,
      std::string data,
      const Proxy& proxy,
      HttpCallbackPtr callback) { asyncPostImpl(url, headers, data, proxy, callback); }
  void asyncDelete(
      const std::string& url,
      const HttpHeaders& headers,
      std::string data,
      const Proxy& proxy,
      HttpCallbackPtr callback) { asyncDeleteImpl(url, headers, data, proxy, callback); }
  void shutdown() { shutdownImpl(); }
};
typedef std::shared_ptr<HttpService> HttpServicePtr;

HttpServicePtr makeHttpService(int numCallbackThreads);

} // namespace sapiremote::http
} // namespace sapiremote

#endif
