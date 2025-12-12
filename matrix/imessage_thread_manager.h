#ifndef MATRIX_I_MESSAGE_THREAD_MANAGER_H
#define MATRIX_I_MESSAGE_THREAD_MANAGER_H

#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <unordered_map>

#include "ithread.h"
#include "imessage.h"
#include "imessage_thread.h"
#include "3rdparty/concurrentqueue/concurrentqueue.h"

namespace mmx {

using std::shared_ptr;
using std::mutex;
using std::vector;
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
  static inline IMessageThreadManager* instance();

  virtual ~IMessageThreadManager();

  int register_thread(IMessageThread *const thread);

  int unregister_thread(IMessageThread *const thread);

  void send_message(const IMessagePtr &msg, uint64_t delay_us);

protected:
  IMessageThreadManager();

  virtual int thread_loop() override;

private:
  static shared_ptr<IMessageThreadManager>  instance_;
  static mutex                              inslock_;

  priority_queue<IMessagePtr,
          vector<IMessagePtr>,
          TimestampCompare>     msg_delayed_queue_;
  ConcurrentQueue<IMessagePtr>  msg_queue_;
  mutex                         msg_lock_;
  int                           msg_delayed_fd_;
  int                           msg_wakeup_fd_;

  unordered_map<pthread_t, IMessageThread*> threads_umap_;
  mutex                                     thread_lock_;
};

} // namespace mmx

#endif // MATRIX_I_MESSAGE_THREAD_MANAGER_H