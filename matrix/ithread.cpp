#include "ithread.h"

#include <cstring>

namespace mmx {

IThread::IThread()
  : thread_exit_pending_(false)
  , thread_running_(false)
  , thread_id_(0)
{

}

IThread::~IThread()
{

}

int IThread::start(const char *name)
{
  if (thread_running_) return 0;
  int ret = -1;

  do {
    std::lock_guard<mutex> _l(thread_lock_);

    thread_exit_pending_ = false;
    thread_running_ = true;
    thread_id_ = 0;

    ret = create_raw_thread(thread_entry_impl, this, name);
    if (ret != 0) {
      thread_running_ = false;
      thread_id_ = 0;
    }
  } while(0);

  return ret;
}

void IThread::stop()
{
  thread_exit_pending_ = true;
}

void IThread::stop_wait()
{
  if (thread_id_ == pthread_self()) {
    fprintf(stderr, "IThread (%s-%p): don't call this function to close self\n",
            thread_name_.c_str(), this);
    return;
  }
  do {
    std::unique_lock<mutex> _l(thread_lock_);
    thread_exit_pending_ = true;
    while (thread_running_) {
      thread_cond_.wait(_l);
    }
    thread_exit_pending_ = false;
  } while(0);
}

int IThread::create_raw_thread(thread_entry entry, void *data, const char *name)
{
  pthread_attr_t attr;
  pthread_t thread;
  int ret;
  errno = 0;

  do {
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&thread, &attr, (void*(*)(void*))entry, data);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
      fprintf(stderr, "IThread::create_raw_thread pthread_create failed:%s\n", strerror(errno));
      break;
    }
    thread_id_ = thread;
    set_thread_name(name);
  } while(0);

  return ret;
}

int IThread::thread_entry_impl(void *data)
{
  IThread *const self = static_cast<IThread*>(data);
  int ret;

  do {
    // if (self->thread_exit_pending_) break;

    ret = self->thread_loop();
    if (ret != 0 || self->thread_exit_pending_) {
      std::lock_guard<mutex> _l(self->thread_lock_);
      self->thread_running_ = false;
      self->thread_id_ = 0;
      self->thread_cond_.notify_all();
      break;
    }
  } while(!self->thread_exit_pending_);

  return 0;
}

int IThread::set_thread_name(const char *name)
{
  thread_name_ = name;
#if defined(__linux__)
  size_t len = strlen(name);
  const char *_name = name;
  if (len > 15) {
    _name = name + len - 15;
  }
  prctl(PR_SET_NAME, (unsigned long)_name, 0, 0, 0);
#endif
  return 0;
}

} // namespace mmx