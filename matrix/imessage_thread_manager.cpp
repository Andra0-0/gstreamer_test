#include "imessage_thread_manager.h"

#include <sys/time.h>
#include <sys/timerfd.h>
#include <cstring>
#include <errno.h>
#include <unistd.h>

#include "debug.h"

namespace mmx {

mutex IMessageThreadManager::inslock_;
shared_ptr<IMessageThreadManager> IMessageThreadManager::instance_ = nullptr;

IMessageThreadManager::IMessageThreadManager()
  : msg_timer_fd_(0)
  , msg_wakeup_fd_(0)
  , msg_epoll_fd_(0)
{
}

IMessageThreadManager::~IMessageThreadManager()
{
}

void IMessageThreadManager::stop()
{
  ALOG_TRACE;
  IThread::stop();

  struct itimerspec value;
  memset(&value, 0, sizeof(value));
  value.it_value.tv_sec = 0;
  value.it_value.tv_nsec = 1; // 1 ns
  timerfd_settime(msg_timer_fd_, 0, &value, NULL);

  msg_cond_.notify_all();
  msg_delayed_cond_.notify_all();

  if (msg_distribute_thread_.joinable()) {
    msg_distribute_thread_.join();
  }
  IThread::join();
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
      std::lock_guard<mutex> _l(msg_delayed_lock_);
      msg_delayed_queue_.emplace(msg);
      update_delayed_timer();
    } else {
      msg_queue_.enqueue(msg);
      msg_cond_.notify_one();
    }
  } while(0);
}

/**
 * Update delayed message timer, set timerfd to next delayed message time
 */
void IMessageThreadManager::update_delayed_timer()
{
  struct itimerspec value;
  memset(&value, 0, sizeof(value));

  if (msg_delayed_queue_.empty()) {
    timerfd_settime(msg_timer_fd_, 0, &value, NULL);
    return;
  }
  auto &msg_top = msg_delayed_queue_.top();

  uint64_t curr_time_ns = get_curr_clock_ns();
  uint64_t delay_ns = 0;

  if (curr_time_ns < msg_top->dst_time()) { // need delay
    delay_ns = msg_top->dst_time() - curr_time_ns;
    if (delay_ns >= 1000000000ULL) {
      value.it_value.tv_sec = delay_ns / 1000000000ULL;
      value.it_value.tv_nsec = delay_ns % 1000000000ULL;
    } else {
      value.it_value.tv_sec = 0;
      value.it_value.tv_nsec = delay_ns;
    }
    timerfd_settime(msg_timer_fd_, 0, &value, NULL);
  } else { // trigger immediately
    value.it_value.tv_sec = 0;
    value.it_value.tv_nsec = 1; // 1 ns
    timerfd_settime(msg_timer_fd_, 0, &value, NULL);
  }
}

int IMessageThreadManager::thread_loop()
{
  ALOG_TRACE;
  epoll_event *events = &msg_epoll_events_;
  int nfds = 0;

  if (thread_init() != 0) {
    ALOGD("IMessageThreadManager thread_init failed!");
    return 0;
  }

  while(!is_exit_pending()) {
    update_delayed_timer();

    nfds = epoll_wait(msg_epoll_fd_, events, 1, -1);

    if (is_exit_pending()) break;
    if (nfds < 0) {
      if (errno == EINTR) { // interrupted by signal, continue
        continue;
      }
      ALOGD("IMessageThreadManager epoll_wait error: %s", strerror(errno));
      break;
    }
    if (nfds > 0 && events[0].data.fd == msg_timer_fd_) { // read timerfd, clear the event
      uint64_t expirations;
      ssize_t s = read(msg_timer_fd_, &expirations, sizeof(uint64_t));
      if (s != sizeof(uint64_t)) {
        ALOGD("IMessageThreadManager read timerfd error: %s", strerror(errno));
        continue;
      }
    }

    if (!msg_delayed_queue_.empty()) { // check delayed message queue, put to message queue
      std::lock_guard<mutex> _l(msg_delayed_lock_);
      uint64_t curr_time_ns = get_curr_clock_ns();
      for (; !msg_delayed_queue_.empty(); ) {
        auto &msg_top = msg_delayed_queue_.top();
        if (curr_time_ns >= msg_top->dst_time()) {
          msg_delayed_queue_.pop();
          msg_queue_.enqueue(std::move(msg_top));
          msg_cond_.notify_one();
        } else { break; }
      }
    }
  }

  thread_deinit();
  return 0;
}

int IMessageThreadManager::thread_init()
{
  int ret = -1;

  do {
    msg_timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    ALOG_BREAK_IF(msg_timer_fd_ < 0);

    msg_epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    ALOG_BREAK_IF(msg_epoll_fd_ < 0);

    msg_epoll_events_.events = EPOLLIN;
    msg_epoll_events_.data.fd = msg_timer_fd_;
    ret = epoll_ctl(msg_epoll_fd_, EPOLL_CTL_ADD, msg_timer_fd_, &msg_epoll_events_);
    if (ret < 0) {
      ALOGD("IMessageThreadManager epoll_ctl add timerfd error: %s", strerror(errno));
      break;
    }

    msg_distribute_thread_ =
            thread(&IMessageThreadManager::distribute_messages, this);
    ALOGD("IMessageThreadManager thread init success");
    ret = 0;
  } while(0);

  return ret;
}

void IMessageThreadManager::thread_deinit()
{
  do {
    epoll_ctl(msg_epoll_fd_, EPOLL_CTL_DEL, msg_timer_fd_, NULL);
    close(msg_timer_fd_);
    close(msg_epoll_fd_);
  } while(0);
}

/**
 * Distribute messages to IMessageThread
 */
void IMessageThreadManager::distribute_messages()
{
  IMessagePtr msg;

  while (!is_exit_pending()) {
    std::unique_lock<mutex> _l(msg_lock_);
    while (msg_queue_.size_approx() == 0 && !is_exit_pending()) {
      msg_cond_.wait(_l);
    }
    if (is_exit_pending()) break;

    while (msg_queue_.size_approx() != 0) { // send message to destination thread
      msg_queue_.try_dequeue(msg);
      if (!msg) continue;

      std::lock_guard<mutex> _tl(thread_lock_);
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
}

}; // namespace mmx