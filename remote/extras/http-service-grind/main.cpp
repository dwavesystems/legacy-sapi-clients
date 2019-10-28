//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <cstdlib>
#include <condition_variable>
#include <iostream>
#include <exception>
#include <memory>
#include <mutex>
#include <string>

#include <http-service.hpp>

using std::atoi;
using std::cerr;
using std::cout;
using std::condition_variable;
using std::exception;
using std::lock_guard;
using std::make_shared;
using std::mutex;
using std::rethrow_exception;
using std::string;
using std::to_string;
using std::unique_lock;

using namespace sapiremote;
using namespace sapiremote::http;

class Callback : public HttpCallback {
  string result_;
  bool done_;
  mutable mutex mutex_;
  mutable condition_variable cv_;

  virtual void completeImpl(int statusCode, std::shared_ptr<std::string> data) {
    lock_guard<mutex> l(mutex_);
    result_ = std::to_string(static_cast<long long>(statusCode));
    done_ = true;
    cv_.notify_all();
  }

  virtual void errorImpl(std::exception_ptr e) {
    lock_guard<mutex> l(mutex_);
    try {
      rethrow_exception(e);
    } catch (exception& ex) {
      result_ = ex.what();
    } catch (...) {
      result_ = "Unknown exception";
    }
    done_ = true;
    cv_.notify_all();
  }

public:
  Callback() : done_(false) {}

  string result() const {
    unique_lock<mutex> lock(mutex_);
    while (!done_) cv_.wait(lock);
    return result_;
  }
};

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <url> [requests=1]\n";
    return 1;
  }

  auto url = string(argv[1]);
  auto requests = atoi(argv[2]);
  if (requests < 1) {
    cerr << "number of requests must be a positive integer\n";
    return 1;
  }

  for (auto i = 0; i < requests; ++i) {
    auto httpService = makeHttpService(2);
    auto callback = make_shared<Callback>();
    cout << i << " ";
    std::flush(cout);
    httpService->asyncGet(url, HttpHeaders(), Proxy(), callback);
    cout << callback->result() << "\n";
  }

  return 0;
}
