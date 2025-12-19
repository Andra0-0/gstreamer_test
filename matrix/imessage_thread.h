#ifndef MATRIX_I_MESSAGE_THREAD_H
#define MATRIX_I_MESSAGE_THREAD_H

#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "ithread.h"
#include "imessage.h"
#include "3rdparty/concurrentqueue/concurrentqueue.h"

namespace mmx {

using std::atomic;
using std::mutex;
using std::condition_variable;
using moodycamel::ConcurrentQueue;

class IMessageThread : public IThread {
public:
  explicit IMessageThread(const char *name);

  virtual ~IMessageThread();

  virtual void stop() override;

  virtual void stop_wait() override;

  friend class IMessageThreadManager;
protected:
  virtual int thread_loop() override;

  virtual void handle_message(const IMessagePtr &msg) = 0;

private:
  ConcurrentQueue<IMessagePtr>  msg_queue_;
  condition_variable            msg_cond_;
  mutex                         msg_lock_;
};

} // namespace mmx

#endif // MATRIX_I_MESSAGE_THREAD_H