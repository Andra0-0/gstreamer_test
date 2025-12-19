#ifndef MATRIX_I_THREAD_H
#define MATRIX_I_THREAD_H

#include <mutex>
#include <string>
#include <atomic>
#include <condition_variable>

#if defined(__linux__)
#include <sys/prctl.h>
#endif

#include "noncopyable.h"

namespace mmx {

typedef int (*thread_entry)(void*);

using std::atomic;
using std::string;
using std::mutex;
using std::condition_variable;

class IThread : public NonCopyable {
public:
  IThread();

  virtual ~IThread();

  virtual int start(const char *name);

  virtual void stop();

  virtual void stop_wait();

  virtual void join();

  virtual pthread_t tid() { return thread_id_; }

  virtual const char *name() { return thread_name_.c_str(); }

  virtual bool is_running() { return thread_running_; }

  virtual bool is_exit_pending() { return thread_exit_pending_; }

protected:
  virtual int thread_loop() = 0;

private:
  int create_raw_thread(thread_entry entry, void *data, const char *name);

  static int thread_entry_impl(void *data);

  int set_thread_name(const char *name);

private:
  atomic<bool>        thread_exit_pending_;
  atomic<bool>        thread_running_;

  pthread_t           thread_id_;
  string              thread_name_;

  condition_variable  thread_cond_;
  mutex               thread_lock_;
};

} // namespace mmx

#endif // MATRIX_I_THREAD_H