/********************************************************************
Copyright 2017 Intel Corp. All Rights Reserved.
Desription : a thread-safe queue
Author     : Wenyi Tang
Email      : wenyi.tang@intel.com
Created    : Nov 21st, 2017
********************************************************************/
#ifndef LL_CODEC_THREAD_SAFE_STL_QUEUE_THREAD_SAFE_QUEUE_H_
#define LL_CODEC_THREAD_SAFE_STL_QUEUE_THREAD_SAFE_QUEUE_H_
#include <assert.h>
#include <deque>
#include <mutex>
#include <queue>


namespace ixr {
template <class Object>
class SafeQueue {
 public:
  SafeQueue() : done_(false) {}

  ~SafeQueue() {
    done_ = true;
    QueueLocker lock(mutex_);
  }

  bool Push(Object &&rhs) {
    QueueLocker lock(mutex_);
    if (done_) return false;
    queue_.push(rhs);
    return true;
  }

  bool Push(const Object &rhs) {
    QueueLocker lock(mutex_);
    if (done_) return false;
    queue_.push(rhs);
    return true;
  }

  bool TryPop(Object *obj = nullptr) {
    QueueLocker lock(mutex_, std::try_to_lock);
    if (lock.owns_lock()) {
      if (queue_.empty()) return false;
      if (obj) *obj = queue_.front();
      queue_.pop();
      return true;
    }
    return false;
  }

  bool WaitPop(Object *obj = nullptr) {
    QueueLocker lock(mutex_);
    assert(lock);
    if (queue_.empty()) return false;
    if (obj) *obj = queue_.front();
    queue_.pop();
    return true;
  }

  bool Empty() {
    QueueLocker lock(mutex_);
    return queue_.empty();
  }

 private:
  bool done_;
  std::queue<Object> queue_;
  std::mutex mutex_;
  using QueueLocker = std::unique_lock<std::mutex>;
};
}  // namespace ixr

#endif  // LL_CODEC_THREAD_SAFE_STL_QUEUE_THREAD_SAFE_QUEUE_H_
