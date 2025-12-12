#include "imessage_thread_manager.h"

#include <sys/time.h>

#include "debug.h"

namespace mmx {

static inline uint64_t get_current_time_us()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

mutex IMessageThreadManager::inslock_;
shared_ptr<IMessageThreadManager> IMessageThreadManager::instance_ = nullptr;

IMessageThreadManager* IMessageThreadManager::instance()
{
  if (instance_ == nullptr) {
    std::lock_guard<mutex> _l(inslock_);
    if (instance_) return instance_.get();

    instance_ = std::make_shared<IMessageThreadManager>();
    instance_->start("IMessageThreadManager");
  }
  return instance_.get();
}

IMessageThreadManager::IMessageThreadManager()
{
}

IMessageThreadManager::~IMessageThreadManager()
{
}

int IMessageThreadManager::register_thread(IMessageThread *const thread)
{
  do {
    std::lock_guard<mutex> _l(thread_lock_);
    threads_umap_.emplace(thread->tid(), thread);
  } while(0);

  return 0;
}

int IMessageThreadManager::unregister_thread(IMessageThread *const thread)
{
  do {
    std::lock_guard<mutex> _l(thread_lock_);
    threads_umap_.erase(thread->tid());
  } while(0);

  return 0;
}

void IMessageThreadManager::send_message(const IMessagePtr &msg, uint64_t delay_us)
{
  do {
    if (delay_us) {
      std::lock_guard<mutex> _l(msg_lock_);
      msg_delayed_queue_.emplace(msg);
    } else {
      msg_queue_.enqueue(msg);
    }
  } while(0);
}

int IMessageThreadManager::thread_loop()
{
  IMessagePtr msg;
  fd_set fdset;
  int fdmax = -1;
  struct timeval tv;

  while(!is_exit_pending()) {
    FD_ZERO(&fdset);
    FD_SET(msg_delayed_fd_, &fdset);
    fdmax = msg_delayed_fd_;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100 ms

    // TODO
    // int ret = select(fdmax + 1, &fdset, NULL, NULL, &tv);
    // if (ret < 0) {
    //   continue;
    // }

    if (!msg_delayed_queue_.empty()) { // check delayed message queue
      std::lock_guard<mutex> _l(msg_lock_);
      for (; !msg_delayed_queue_.empty(); ) {
        auto &msg_top = msg_delayed_queue_.top();
        if (msg_top->dst_time() <= get_current_time_us()) {
          msg_delayed_queue_.pop();
          msg_queue_.enqueue(std::move(msg_top));
        } else {
          break;
        }
      }
    }
    while (msg_queue_.size_approx() != 0) { // send message to destination thread
      msg_queue_.try_dequeue(msg);
      if (!msg) continue;

      std::lock_guard<mutex> _l(thread_lock_);
      auto it = threads_umap_.find(msg->dst_tid());
      if (it != threads_umap_.end()) {
        if (it->second->msg_queue_.size_approx() < 100) {
          it->second->msg_queue_.enqueue(std::move(msg));
          it->second->msg_cond_.notify_one();
        } else {
          ALOGD("IMessageThread message queue too long!");
        }
      }
      msg = nullptr;
    }
  }

  return 0;
}

}; // namespace mmx