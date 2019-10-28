//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <string>
#include <sstream>
#include <thread>

#include <iostream>

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/system/error_code.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <curl/curl.h>

#include <exceptions.hpp>
#include <http-service.hpp>
#include <threadpool.hpp>

using std::size_t;
using std::bad_alloc;
using std::bind;
using std::map;
using std::set;
using std::string;
using std::ostringstream;
using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using std::make_shared;
using std::ref;
using std::thread;
using std::mutex;
using std::recursive_mutex;
using std::lock_guard;
using std::unique_lock;
using std::enable_shared_from_this;
using std::exception_ptr;
using std::current_exception;
using namespace std::placeholders;

using boost::asio::io_service;
using boost::asio::deadline_timer;
using boost::posix_time::milliseconds;

using sapiremote::http::HttpService;
using sapiremote::http::HttpHeaders;
using sapiremote::http::Proxy;
using sapiremote::http::HttpCallback;
using sapiremote::http::HttpCallbackPtr;
using sapiremote::ServiceShutdownException;
using sapiremote::ThreadPoolPtr;
using sapiremote::makeThreadPool;

namespace {

namespace verbs {
const char* delete_ = "DELETE";
}

class CurlMultiService;


//=========================================================================================================
//
// libcurl exceptions
//


string curlcodeMsg(CURLcode c);
string curlmcodeMsg(CURLMcode c);

class CurlException : public sapiremote::NetworkException {
public:
  CurlException(CURLcode c) : NetworkException(curlcodeMsg(c)) {}
};


class CurlMultiException : public sapiremote::NetworkException {
public:
  CurlMultiException(CURLMcode c) : NetworkException(curlmcodeMsg(c)) {}
};

void handleCurlError(CURLcode c);
void handleCurlMultiError(CURLMcode c);

//=========================================================================================================
//
// libcurl global initialization/cleanup
//

class CurlGlobal {
private:
  CURLcode result_;
public:
  CurlGlobal() : result_(curl_global_init(CURL_GLOBAL_ALL)) {}
  ~CurlGlobal() { curl_global_cleanup(); }
  void check() const { if (result_ != CURLE_OK) throw CurlException(result_); }
};
const CurlGlobal curlGlobal;

//=========================================================================================================
//
// libcurl headers
//

class CurlHeaders : boost::noncopyable {
private:
  curl_slist* headers_;
public:
  CurlHeaders() : headers_(0) {}
  CurlHeaders(CurlHeaders&& other) : headers_(other.headers_) { other.headers_ = 0; }
  CurlHeaders& operator=(CurlHeaders&& other) { std::swap(headers_, other.headers_); return *this; }
  ~CurlHeaders() { curl_slist_free_all(headers_); }
  void append(const char* header) {
    curl_slist* newHeaders = curl_slist_append(headers_, header);
    if (!newHeaders) throw bad_alloc();
    headers_ = newHeaders;
  }
  curl_slist* get() { return headers_; }
};

CurlHeaders convertHeaders(const HttpHeaders& headers) {
  CurlHeaders curlHeaders;
  for(HttpHeaders::const_iterator iter = headers.begin(), end = headers.end(); iter != end; ++iter ) {
    string headerLine = iter->first + ": " + iter->second;
    curlHeaders.append(headerLine.c_str());
  }
  return curlHeaders;
}

//=========================================================================================================
//
// io_service thread

class IoServiceThread : boost::noncopyable {
private:
  thread t_;

  static void run(io_service& ioService) { ioService.run(); }

public:
  IoServiceThread() {}
  IoServiceThread(IoServiceThread&& other) : t_(std::move(other.t_)) {}
  IoServiceThread& operator=(IoServiceThread&& other) { t_ = std::move(other.t_); return *this; }
  IoServiceThread(io_service& ioService) : t_(run, ref(ioService)) {}
  ~IoServiceThread() { join(); }

  void join() { if (t_.joinable()) t_.join(); }
};

//=========================================================================================================
//
// Http callback service

class HttpCallbackService {
private:
  ThreadPoolPtr threadpool_;
public:
  HttpCallbackService(ThreadPoolPtr threadpool) : threadpool_(threadpool) {}
  void postComplete(HttpCallbackPtr callback, int statusCode, shared_ptr<string> data) {
    try {
      threadpool_->post(bind(&HttpCallback::complete, callback, statusCode, data));
    } catch (...) {
      postError(callback, current_exception());
    }
  }
  void postError(HttpCallbackPtr callback, exception_ptr e) {
    try {
      threadpool_->post(bind(&HttpCallback::error, callback, e));
    } catch (...) {
      callback->error(current_exception());
    }
  }
  void shutdown() { threadpool_->shutdown(); }
};

//=========================================================================================================
//
// HTTP connection
//
// libcurl easy interface (i.e. single HTTP request) details are mostly hidden in here.
// Needs access to CurlMultiService for socket open/close callbacks


class Connection : public enable_shared_from_this<Connection>, boost::noncopyable {
private:
  static const long connectTimeoutS = 30;
  static const long lowSpeedTimeoutS = 30;
  static size_t writeFunction(char* ptr, size_t size, size_t nmemb, void* userdata);
  static size_t headerFunction(void* ptr, size_t size, size_t nmemb, void* userdata);

  struct CleanupEasyHandle { void operator()(CURL* h) { curl_easy_cleanup(h); } };
  enum State { IN_PROGRESS, PENDING_COMPLETE, PENDING_FAIL, FINISHED };

  HttpCallbackService callbackService_;
  unique_ptr<CURL, CleanupEasyHandle> easyHandle_;
  CurlHeaders curlHeaders_;
  shared_ptr<string> writeBuffer_;
  string readBuffer_;
  char errorBuffer_[CURL_ERROR_SIZE];
  HttpCallbackPtr callback_;
  State complete_;
  CURLcode code_;
  exception_ptr exc_;
  shared_ptr<Connection> next_;

  template<typename T>
  void setopt(CURLoption option, T value);
  void setOptions(const string& url, const Proxy& proxy, CurlMultiService* curlMulti);

  void complete(CURLcode c);
  void fail(exception_ptr e);

public:
  Connection(
      HttpCallbackService callbackService,
      CurlMultiService& curlMulti,
      HttpCallbackPtr callback,
      const string& url,
      const HttpHeaders& headers,
      const Proxy& proxy);
  Connection(
      HttpCallbackService callbackService,
      CurlMultiService& curlMulti,
      HttpCallbackPtr callback,
      const char* verb,
      const string& url,
      const HttpHeaders& headers,
     std::string& data,
     const Proxy& proxy);
  Connection(
      HttpCallbackService callbackService,
      CurlMultiService& curlMulti,
      HttpCallbackPtr callback,
      const string& url,
      const HttpHeaders& headers,
      std::string& data,
      const Proxy& proxy);
  ~Connection();
  void queueComplete(CURLcode c, shared_ptr<Connection> next);
  void queueFail(exception_ptr e, shared_ptr<Connection> next);
  shared_ptr<Connection> next() { return next_; }
  void finish();
  CURL* rawHandle() { return easyHandle_.get(); }
};
typedef shared_ptr<Connection> ConnectionPtr;

ConnectionPtr curlToConnection(CURL* curl);

ConnectionPtr httpGetConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    const Proxy& proxy);

ConnectionPtr httpPostConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy);

ConnectionPtr httpDeleteConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy);


//=========================================================================================================
//
// Socket
//

class Socket : public enable_shared_from_this<Socket>, boost::noncopyable {
private:
  boost::asio::ip::tcp::socket socket_;
  int action_;
public:
  Socket(io_service& ioService, boost::asio::ip::tcp tcpSettings) :
    socket_(ioService, tcpSettings), action_(CURL_POLL_NONE) {}
  boost::asio::ip::tcp::socket& asioSocket() { return socket_; }
  curl_socket_t nativeSocket() { return socket_.native_handle(); }
  void close(boost::system::error_code& ec) { socket_.close(ec); }
  int setAction(int newAction);
  int eventComplete(int evBitmask);
};
typedef shared_ptr<Socket> SocketPtr;



//=========================================================================================================
//
// libcurl multi interface

class CurlMultiService : boost::noncopyable {
private:
  typedef map<curl_socket_t, SocketPtr> SocketMap;
  typedef set<ConnectionPtr> ConnectionSet;
  typedef mutex Mutex;

  struct CleanupMultiHandle { void operator()(CURLM* h) { curl_multi_cleanup(h); } };

  unique_ptr<io_service> ioService_;
  IoServiceThread thread_;
  deadline_timer timer_;
  SocketMap sockets_;
  unique_ptr<CURLM, CleanupMultiHandle> multiHandle_;
  ConnectionSet activeConnections_;
  Mutex mutex_;
  ConnectionPtr doneHead_;
  unique_ptr<io_service::work> work_;
  bool running_;

  // libcurl callback implementations -- no locking
  void socketCallback(CURL* easyHandle, curl_socket_t s, int action);
  void setSocketActionTimer(long timeoutMs);
  curl_socket_t openSocket(boost::asio::ip::tcp tcpSettings);
  int closeSocket(curl_socket_t s);
  // ------

  // no locking (just calls locking socketAction)
  void timerExpired(const boost::system::error_code& ec);

  // no locking
  SocketPtr socketFromNative(curl_socket_t s);

  // no locking
  void asyncSocketIo(CURL* easyHandle, SocketPtr s, int action);

  // called by constructor only, no locking necessary
  template<typename T>
  void setopt(CURLMoption option, T value);

  // called by socketActionUnlocked, no locking necessary
  void processCompletedConnections();

  // no locking
  void completeConnection(ConnectionPtr conn, CURLcode c);
  void failConnection(ConnectionPtr conn, exception_ptr e);
  void removeConnection(ConnectionPtr conn);

  // must be called while unlocked!
  void invokeCompletedCallbacks();

public:
  // libcurl callback static wrappers
  // --------------------------------
  //  These are only invoked as callbacks from curl_multi_* functions, which are in turn only
  //  invoked by other CurlMultiService member functions.  Those member functions are responsible for
  //  locking the member mutex so no locking is done here.
  static int curlSocketCallback(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);
  static int curlTimerCallback(CURLM *multi, long timeoutMs, void *userp);
  static curl_socket_t curlOpenSocket(void* clientp, curlsocktype purpose, curl_sockaddr* address);
  static int curlCloseSocket(void* clientp, curl_socket_t item);
  // ------

  CurlMultiService();
  ~CurlMultiService();

  void shutdown();

  // external entry point -- locks mutex_
  void addConnection(ConnectionPtr conn);

  // io_service callback function -- locks mutex_
  void socketAction(CURL* easyHandle, curl_socket_t s, int ev_bitmask, const boost::system::error_code& ec);
};


//=========================================================================================================
//
// HttpService implementation
//

class HttpServiceImpl : public HttpService, boost::noncopyable {
private:
  HttpCallbackService callbackService_;
  CurlMultiService curlMultiService_;

  virtual void asyncGetImpl(
      const std::string& url,
      const HttpHeaders& headers,
      const Proxy& proxy,
      HttpCallbackPtr callback) {
    ConnectionPtr conn = httpGetConnection(callbackService_, curlMultiService_, callback, url, headers, proxy);
    curlMultiService_.addConnection(conn);
  }

  virtual void asyncPostImpl(
      const std::string& url,
      const HttpHeaders& headers,
      std::string& data,
      const Proxy& proxy,
      HttpCallbackPtr callback) {
    ConnectionPtr conn = httpPostConnection(callbackService_, curlMultiService_, callback, url, headers, data, proxy);
    curlMultiService_.addConnection(conn);
  }

  virtual void asyncDeleteImpl(
      const std::string& url,
      const HttpHeaders& headers,
      std::string& data,
      const Proxy& proxy,
      HttpCallbackPtr callback) {
    ConnectionPtr conn = httpDeleteConnection(
        callbackService_, curlMultiService_, callback, url, headers, data, proxy);
    curlMultiService_.addConnection(conn);
  }

  virtual void shutdownImpl() {
    curlMultiService_.shutdown();
    callbackService_.shutdown();
  }

public:
  HttpServiceImpl(int numCallbackThreads) :
      callbackService_(makeThreadPool(numCallbackThreads)),
      curlMultiService_() {
    curlGlobal.check();
  }

  virtual ~HttpServiceImpl() { shutdown(); }
};



//=========================================================================================================
//
// libcurl exception message implementation
//

void handleCurlError(CURLcode c) {
  switch (c) {
    case CURLE_OK:
      return;
    case CURLE_OUT_OF_MEMORY:
      throw std::bad_alloc();
    default:
      throw CurlException(c);
  }
}

void handleCurlMultiError(CURLMcode c) {
  switch (c) {
    case CURLM_OK:
      return;
    case CURLM_OUT_OF_MEMORY:
      throw std::bad_alloc();
    default:
      throw CurlMultiException(c);
  }
}

string curlcodeMsg(CURLcode c) {
  ostringstream msg;
  msg << curl_easy_strerror(c) << " (error code " << c << ")";
  return msg.str();
}

string curlmcodeMsg(CURLMcode c) {
  ostringstream msg;
  msg << curl_multi_strerror(c) << " (error code " << c << ")";
  return msg.str();
}

//=========================================================================================================
//
// CurlMultiService implementation
//

CurlMultiService::CurlMultiService() :
ioService_(new io_service),
    work_(new io_service::work(*ioService_)),
    timer_(*ioService_),
    multiHandle_(curl_multi_init()),
    mutex_(),
    running_(true) {

  if (!multiHandle_) throw bad_alloc();
  setopt(CURLMOPT_SOCKETFUNCTION, &CurlMultiService::curlSocketCallback);
  setopt(CURLMOPT_SOCKETDATA, this);
  setopt(CURLMOPT_TIMERFUNCTION, &CurlMultiService::curlTimerCallback);
  setopt(CURLMOPT_TIMERDATA, this);

  thread_ = IoServiceThread(*ioService_);
}

CurlMultiService::~CurlMultiService() {
  shutdown();
}

template<typename T>
void CurlMultiService::setopt(CURLMoption option, T value) {
  CURLMcode r = curl_multi_setopt(multiHandle_.get(), option, value);
  if (r != CURLM_OK) throw CurlMultiException(r);
}

void CurlMultiService::processCompletedConnections() {
  int m;
  for (CURLMsg* msg = curl_multi_info_read(multiHandle_.get(), &m);
      msg;
      msg = curl_multi_info_read(multiHandle_.get(), &m)) {

    if (msg->msg != CURLMSG_DONE) continue;
    ConnectionPtr conn = curlToConnection(msg->easy_handle);
    completeConnection(conn, msg->data.result);
  }
}

void CurlMultiService::completeConnection(ConnectionPtr conn, CURLcode c) {
  conn->queueComplete(c, doneHead_);
  doneHead_ = conn;
}

void CurlMultiService::failConnection(ConnectionPtr conn, exception_ptr e) {
  conn->queueFail(e, doneHead_);
  doneHead_ = conn;
}

void CurlMultiService::removeConnection(ConnectionPtr conn) {
  auto iter = activeConnections_.find(conn);
  if (iter != activeConnections_.end()) activeConnections_.erase(iter);
  curl_multi_remove_handle(multiHandle_.get(), conn->rawHandle());
}

void CurlMultiService::invokeCompletedCallbacks() {
  ConnectionPtr head;
  {
    lock_guard<Mutex> lock(mutex_);
    head.swap(doneHead_);

    for (auto c = head; c; c = c->next()) {
      removeConnection(c);
    }
  }

  if (head) head->finish();
}

void CurlMultiService::timerExpired(const boost::system::error_code& ec) {
  if (!ec) socketAction(0, CURL_SOCKET_TIMEOUT, 0, boost::system::error_code());
}

void CurlMultiService::socketAction(
    CURL* easyHandle, curl_socket_t s, int evBitmask, const boost::system::error_code& ec) {
  if (!ec) {
    {
      lock_guard<Mutex> lock(mutex_);
      int runningHandles;
      CURLMcode r = curl_multi_socket_action(multiHandle_.get(), s, evBitmask, &runningHandles);
      if (s != CURL_SOCKET_TIMEOUT) {
        SocketPtr sp = socketFromNative(s);
        if (sp) asyncSocketIo(easyHandle, sp, sp->eventComplete(evBitmask));
      }
      if (r == CURLM_OK && runningHandles < static_cast<int>(activeConnections_.size())) processCompletedConnections();
    }
    invokeCompletedCallbacks();
  }
}

int CurlMultiService::curlSocketCallback(CURL* easyHandle, curl_socket_t s, int action, void* userp, void*) {
  try {
    static_cast<CurlMultiService*>(userp)->socketCallback(easyHandle, s, action);
  } catch (...) {}
  return 0;
}

void CurlMultiService::socketCallback(CURL* easyHandle, curl_socket_t s, int action) {
  SocketPtr socket = socketFromNative(s);
  if (socket) asyncSocketIo(easyHandle, socket, socket->setAction(action));
}



int CurlMultiService::curlTimerCallback(CURLM *, long timeoutMs, void *userp) {
  try {
    static_cast<CurlMultiService*>(userp)->setSocketActionTimer(timeoutMs);
    return 0;
  } catch (...) {}
  return -1;
}

void CurlMultiService::setSocketActionTimer(long timeoutMs) {
  if (timeoutMs >= 0) {
    timer_.expires_from_now(milliseconds(timeoutMs));
    timer_.async_wait(bind(&CurlMultiService::timerExpired, this, _1));
  } else {
    timer_.cancel();
  }
}



curl_socket_t CurlMultiService::curlOpenSocket(void* clientp, curlsocktype purpose, curl_sockaddr* address) {
  static const boost::asio::ip::tcp v4 = boost::asio::ip::tcp::v4();
  static const boost::asio::ip::tcp v6 = boost::asio::ip::tcp::v6();
  try {
    if (purpose != CURLSOCKTYPE_IPCXN) return CURL_SOCKET_BAD;
    if (address->family == v4.family()) {
      if (address->socktype != v4.type()) return CURL_SOCKET_BAD;
      return static_cast<CurlMultiService*>(clientp)->openSocket(v4);
    } else if (address->family == v6.family()) {
      if (address->socktype != v6.type()) return CURL_SOCKET_BAD;
      return static_cast<CurlMultiService*>(clientp)->openSocket(v6);
    }
  } catch (...) {}
  return CURL_SOCKET_BAD;
}

curl_socket_t CurlMultiService::openSocket(boost::asio::ip::tcp tcpSettings) {
  for (;;) {
    SocketPtr socketPtr = make_shared<Socket>(ref(*ioService_), tcpSettings);
    curl_socket_t nativeSocket = socketPtr->nativeSocket();

    // When libcurl uses NSS for https, NSS seems to hijack closing sockets
    // So we might have stale entries in sockets_ which might have just been revived...
    // only to be destroyed again when old sockets_[nativeSocket] is replaced
    // If existing entry is found, erase it and drop newly created socket, then try again
    auto iter = sockets_.find(nativeSocket);
    if (iter == sockets_.end()) {
      sockets_[nativeSocket] = socketPtr;
      return nativeSocket;
    } else {
      sockets_.erase(iter);
    }
  }
}



int CurlMultiService::curlCloseSocket(void* clientp, curl_socket_t s) {
  try {
    return static_cast<CurlMultiService*>(clientp)->closeSocket(s);
  } catch (...) {}
  return 1;
}

int CurlMultiService::closeSocket(curl_socket_t s) {
  SocketMap::iterator iter = sockets_.find(s);
  if (iter == sockets_.end()) return 1;
  boost::system::error_code ec;
  iter->second->close(ec);
  sockets_.erase(iter);
  return ec ? 1 : 0;
}

SocketPtr CurlMultiService::socketFromNative(curl_socket_t s) {
  SocketMap::iterator iter = sockets_.find(s);
  if (iter != sockets_.end()) {
    return iter->second;
  } else {
    return SocketPtr();
  }
}

void CurlMultiService::asyncSocketIo(CURL* easyHandle, SocketPtr sp, int action) {
  try {
    if (action & CURL_POLL_IN) {
      sp->asioSocket().async_read_some(boost::asio::null_buffers(),
          bind(&CurlMultiService::socketAction, this,
              easyHandle, sp->nativeSocket(), CURL_CSELECT_IN, _1));
    }
    if (action & CURL_POLL_OUT) {
      sp->asioSocket().async_write_some(boost::asio::null_buffers(),
          bind(&CurlMultiService::socketAction, this,
              easyHandle, sp->nativeSocket(), CURL_CSELECT_OUT, _1));
    }
  } catch(...) {
    ConnectionPtr conn = curlToConnection(easyHandle);
    failConnection(conn, current_exception());
  }
}

void CurlMultiService::addConnection(ConnectionPtr conn) {
  lock_guard<Mutex> lock(mutex_);
  if (!running_) throw ServiceShutdownException();
  handleCurlMultiError(curl_multi_add_handle(multiHandle_.get(), conn->rawHandle()));
  activeConnections_.insert(conn);
  //  setSocketActionTimer(0);
}

void CurlMultiService::shutdown() {
  // fail all active connections and invoke their callbacks
  {
    lock_guard<Mutex> lock(mutex_);
    running_ = false; // no new connections!
    for (auto iter = activeConnections_.begin(), end = activeConnections_.end(); iter != end; ++iter) {
      try {
        throw ServiceShutdownException();
      }
      catch (...) {
        failConnection(*iter, current_exception());
      }
    }
  }
  invokeCompletedCallbacks();

  // at this point, nothing is happening. Time to clean up curl multi handle and shutdown threads
  // locking is unnecessary since running_ is false.
  multiHandle_.reset();
  sockets_.clear();
  timer_.cancel();
  work_.reset();
  thread_.join();
  ioService_.reset();
}


//=========================================================================================================
//
// Socket implementation
//

int Socket::setAction(int newAction) {
  int newBits = ~action_ & newAction;
  action_ = newAction;
  return newBits;
}

int Socket::eventComplete(int evBitmask) {
  return ((action_ & CURL_POLL_IN ? CURL_CSELECT_IN : 0)
      | (action_ & CURL_POLL_OUT ? CURL_CSELECT_OUT : 0)) & evBitmask;
}


//=========================================================================================================
//
// Connection implementation
//

ConnectionPtr curlToConnection(CURL* curl) {
  Connection* rp;
  CURLcode r = curl_easy_getinfo(curl, CURLINFO_PRIVATE, reinterpret_cast<char**>(&rp));
  if (r != CURLE_OK) throw CurlException(r);
  return rp->shared_from_this();
}

size_t Connection::writeFunction(char* ptr, size_t size, size_t nmemb, void* userdata) {
  size_t bytes = size * nmemb;
  try {
    auto r = static_cast<Connection*>(userdata)->shared_from_this();
    r->writeBuffer_->append(ptr, ptr + bytes);
    return bytes;
  } catch (...) {}
  return bytes - 1;
}

size_t Connection::headerFunction(void*, size_t size, size_t nmemb, void*) {
  return size * nmemb;
}

Connection::Connection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMulti,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    const Proxy& proxy) :
        callbackService_(callbackService),
        easyHandle_(curl_easy_init()),
        curlHeaders_(convertHeaders(headers)),
        writeBuffer_(make_shared<string>()),
        callback_(callback),
        complete_(IN_PROGRESS) {

  setOptions(url, proxy, &curlMulti);
}

Connection::Connection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMulti,
    HttpCallbackPtr callback,
    const char* verb,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy) :
        callbackService_(callbackService),
        easyHandle_(curl_easy_init()),
        curlHeaders_(convertHeaders(headers)),
        writeBuffer_(make_shared<string>()),
        readBuffer_(std::move(data)),
        callback_(callback),
        complete_(IN_PROGRESS) {

  setOptions(url, proxy, &curlMulti);
  setopt(CURLOPT_CUSTOMREQUEST, verb);
  setopt(CURLOPT_POST, 1L);
  setopt(CURLOPT_POSTFIELDS, readBuffer_.data());
  setopt(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(readBuffer_.size()));
}

Connection::Connection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMulti,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy) :
        callbackService_(callbackService),
        easyHandle_(curl_easy_init()),
        curlHeaders_(convertHeaders(headers)),
        writeBuffer_(make_shared<string>()),
        readBuffer_(std::move(data)),
        callback_(callback),
        complete_(IN_PROGRESS) {

  setOptions(url, proxy, &curlMulti);
  setopt(CURLOPT_POST, 1L);
  setopt(CURLOPT_POSTFIELDS, readBuffer_.data());
  setopt(CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(readBuffer_.size()));
}

Connection::~Connection() {
  try {
    finish();
  } catch (...) {
    std::terminate();
  }
}


template <typename T>
void Connection::setopt(CURLoption option, T value) {
  handleCurlError(curl_easy_setopt(easyHandle_.get(), option, value));
}

void Connection::setOptions(const string& url, const Proxy& proxy, CurlMultiService* curlMulti) {
  setopt(CURLOPT_NOSIGNAL, 1L);
  setopt(CURLOPT_WRITEFUNCTION, &Connection::writeFunction);
  setopt(CURLOPT_WRITEDATA, this);
  setopt(CURLOPT_OPENSOCKETFUNCTION, &CurlMultiService::curlOpenSocket);
  setopt(CURLOPT_OPENSOCKETDATA, curlMulti);
  setopt(CURLOPT_HEADERFUNCTION, &Connection::headerFunction);
  setopt(CURLOPT_CLOSESOCKETFUNCTION, &CurlMultiService::curlCloseSocket);
  setopt(CURLOPT_CLOSESOCKETDATA, curlMulti);
  setopt(CURLOPT_ERRORBUFFER, static_cast<char*>(errorBuffer_));
  setopt(CURLOPT_URL, url.c_str());
  setopt(CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
  setopt(CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
  if (proxy.enabled()) setopt(CURLOPT_PROXY, proxy.url().c_str());
  setopt(CURLOPT_ACCEPT_ENCODING, ""); // empty causes libcurl to list all supported encodings
  setopt(CURLOPT_HTTPHEADER, curlHeaders_.get());
  setopt(CURLOPT_FOLLOWLOCATION, 1L);
  setopt(CURLOPT_MAXREDIRS, 128L); //
  setopt(CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
  setopt(CURLOPT_CONNECTTIMEOUT, connectTimeoutS);
  setopt(CURLOPT_LOW_SPEED_LIMIT, 1); // time out connection if bytes stop arriving (usually triggered by firewalls)
  setopt(CURLOPT_LOW_SPEED_TIME , lowSpeedTimeoutS);
  setopt(CURLOPT_SSL_VERIFYPEER, 0L); // less secure but app servers lack proper certificates
  setopt(CURLOPT_SSL_VERIFYHOST, 0L);
  setopt(CURLOPT_PRIVATE, this);
}

void Connection::complete(CURLcode c) {
  complete_ = FINISHED;
  long responseCode = 0;
  if (c == CURLE_OK) c = curl_easy_getinfo(easyHandle_.get(), CURLINFO_RESPONSE_CODE, &responseCode);
  if (c == CURLE_OK) {
    callbackService_.postComplete(callback_, static_cast<int>(responseCode), writeBuffer_);
    callback_.reset();
  } else {
    try {
      throw CurlException(c);
    } catch (...) {
      fail(current_exception());
    }
  }
}

void Connection::fail(exception_ptr e) {
  complete_ = FINISHED;
  callbackService_.postError(callback_, e);
  callback_.reset();
}

void Connection::queueComplete(CURLcode c, ConnectionPtr next) {
  if (complete_ == IN_PROGRESS) {
    code_ = c;
    complete_ = PENDING_COMPLETE;
    next_ = next;
  }
}

void Connection::queueFail(exception_ptr e, ConnectionPtr next) {
  if (complete_ == IN_PROGRESS) {
    exc_ = e;
    complete_ = PENDING_FAIL;
    next_ = next;
  }
}

void Connection::finish() {
  if (next_) {
    next_->finish();
    next_.reset();
  }
  switch (complete_) {
    case PENDING_COMPLETE: complete(code_); break;
    case PENDING_FAIL: fail(exc_); break;
    case FINISHED: break;
    default: break;
  }
}

ConnectionPtr httpGetConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    const Proxy& proxy) {
  return make_shared<Connection>(callbackService, curlMultiService, callback, url, headers, proxy);
}

ConnectionPtr httpPostConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy) {
  return make_shared<Connection>(callbackService, curlMultiService, callback, url, headers, ref(data), proxy);
}

ConnectionPtr httpDeleteConnection(
    HttpCallbackService callbackService,
    CurlMultiService& curlMultiService,
    HttpCallbackPtr callback,
    const string& url,
    const HttpHeaders& headers,
    std::string& data,
    const Proxy& proxy) {
  return make_shared<Connection>(
      callbackService, curlMultiService, callback, verbs::delete_, url, headers, ref(data), proxy);
}

} // namespace {anonymous}

namespace sapiremote {
namespace http {
HttpServicePtr makeHttpService(int numCallbackThreads) {
  return make_shared<HttpServiceImpl>(numCallbackThreads);
}
} // namespace sapi::http
} // namespace sapi
