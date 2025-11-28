#include "console_interface.h"

namespace mmx {

ConsoleInterfacePtr ConsoleInterface::instance()
{
  if (!instance_) {
    std::lock_guard<mutex> _l(ins_lock_);
    if (instance_) return instance_;

    instance_ = std::make_shared<ConsoleInterface>();
  }
  return instance_;
}

int ConsoleInterface::thread_loop()
{

}

}