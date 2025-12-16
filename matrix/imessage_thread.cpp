#include "imessage_thread.h"

#include "imessage_thread_manager.h"
#include "debug.h"

namespace mmx {

/**
 * IMessageThread constructor
 * @param name Thread name, max length 15 characters
 */
IMessageThread::IMessageThread(const char *name)
{
  do {
    ALOG_BREAK_IF(IThread::start(name) != 0);
    IMessageThreadManager::instance()->register_thread(this);
  } while(0);
}

IMessageThread::~IMessageThread()
{
  do {
    IMessageThreadManager::instance()->unregister_thread(this);
    IThread::stop();
    msg_cond_.notify_all();
    IThread::stop_wait();
  } while(0);
}

int IMessageThread::thread_loop()
{
  IMessagePtr msg;

  while (!is_exit_pending()) {
    {
      std::unique_lock<mutex> _l(msg_lock_);
      while (msg_queue_.size_approx() == 0 && !is_exit_pending()) {
        msg_cond_.wait(_l);
      }
      if (is_exit_pending()) break;
      msg_queue_.try_dequeue(msg);
    }

    if (msg) {
      handle_message(msg);
      msg = nullptr;
    }
  }

  return 0;
}

}; // namespace mmx