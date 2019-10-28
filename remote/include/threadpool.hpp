//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#ifndef THREADPOOL_HPP_INCLUDED
#define THREADPOOL_HPP_INCLUDED

#include <functional>
#include <memory>

namespace sapiremote {

class ThreadPool {
private:
  virtual void shutdownImpl() = 0;
  virtual void postImpl(std::function<void()> f) = 0;

public:
  virtual ~ThreadPool() {}

  void shutdown() { shutdownImpl(); }
  void post(std::function<void()> f) { postImpl(f); }
};
typedef std::shared_ptr<ThreadPool> ThreadPoolPtr;

ThreadPoolPtr makeThreadPool(int threads);

} // namespace sapiremote

#endif
