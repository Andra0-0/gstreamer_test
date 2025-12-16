#ifndef MATRIX_I_MESSAGE_THREAD_MANAGER_H
#define MATRIX_I_MESSAGE_THREAD_MANAGER_H

#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <unordered_map>
#include <sys/epoll.h>

#include "ithread.h"
#include "imessage.h"
#include "imessage_thread.h"
#include "3rdparty/concurrentqueue/concurrentqueue.h"

namespace mmx {

using std::shared_ptr;
using std::mutex;
using std::vector;
using std::thread;
using std::unordered_map;
using std::priority_queue;
using moodycamel::ConcurrentQueue;

struct TimestampCompare {
  bool operator()(const IMessagePtr &a, const IMessagePtr &b) const {
    return a->dst_time() > b->dst_time();
  }
};

class IMessageThreadManager : public IThread {
public:
  static inline IMessageThreadManager* instance() {
    if (instance_ == nullptr) {
      std::lock_guard<mutex> _l(inslock_);
      if (instance_) return instance_.get();

      instance_ = shared_ptr<IMessageThreadManager>(new IMessageThreadManager());
      instance_->start("MsgThrManager");
    }
    return instance_.get();
  }

  virtual ~IMessageThreadManager();

  virtual void stop() override;

  int register_thread(IMessageThread *const thread);

  int unregister_thread(IMessageThread *const thread);

  void send_message(const IMessagePtr &msg, uint64_t delay_us);

  static inline uint64_t get_curr_clock_us() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000ULL + now.tv_nsec / 1000;
  }

  static inline uint64_t get_curr_clock_ns() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000ULL + now.tv_nsec;
  }

protected:
  IMessageThreadManager();

  void update_delayed_timer();

  virtual int thread_loop() override;

  int thread_init();

  void thread_deinit();

  void distribute_messages();

private:
  static shared_ptr<IMessageThreadManager>  instance_;
  static mutex                              inslock_;

  priority_queue<IMessagePtr,
          vector<IMessagePtr>,
          TimestampCompare>     msg_delayed_queue_;
  ConcurrentQueue<IMessagePtr>  msg_queue_;
  thread                        msg_distribute_thread_;
  condition_variable            msg_delayed_cond_;
  mutex                         msg_delayed_lock_;
  condition_variable            msg_cond_;
  mutex                         msg_lock_;
  int                           msg_timer_fd_;
  int                           msg_epoll_fd_;
  epoll_event                   msg_epoll_events_;

  unordered_map<pthread_t, IMessageThread*> threads_umap_;
  mutex                                     thread_lock_;
};

} // namespace mmx

#endif // MATRIX_I_MESSAGE_THREAD_MANAGER_H