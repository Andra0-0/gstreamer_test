#include "console_interface.h"

#include "debug.h"

namespace mmx {

mutex ConsoleInterface::ins_lock_;
ConsoleInterfacePtr ConsoleInterface::instance_ = ConsoleInterface::instance();

ConsoleInterfacePtr ConsoleInterface::instance()
{
  if (!instance_) {
    std::lock_guard<mutex> _l(ins_lock_);
    if (instance_) return instance_;

    instance_ = std::make_shared<ConsoleInterface>();
    instance_->start("ConsoleInterface");
  }
  return instance_;
}

int ConsoleInterface::thread_loop()
{
  if (thread_init() != 0) {
    return 0;
  }

  while (!is_exit_pending()) {

  }

  thread_deinit();
  return 0;
}

int ConsoleInterface::thread_init()
{
  return 0;
}

int ConsoleInterface::thread_deinit()
{
  return 0;
}

}