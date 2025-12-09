#include "console_terminal.h"

#include <unistd.h>
#include "debug.h"

namespace mmx {

mutex ConsoleTerminal::ins_lock_;
ConsoleTerminalPtr ConsoleTerminal::instance_ = ConsoleTerminal::instance();

ConsoleTerminalPtr ConsoleTerminal::instance()
{
  if (!instance_) {
    std::lock_guard<mutex> _l(ins_lock_);
    if (instance_) return instance_;

    instance_ = std::make_shared<ConsoleTerminal>();
    instance_->start("ConsoleTerminal");
  }
  return instance_;
}

int ConsoleTerminal::thread_loop()
{
  if (thread_init() != 0) {
    return 0;
  }

  while (!is_exit_pending()) {

  }

  thread_deinit();
  return 0;
}

int ConsoleTerminal::thread_init()
{
  return 0;
}

int ConsoleTerminal::thread_deinit()
{
  return 0;
}

} // namespace mmx

int main(int argc, char *argv[])
{
  ALOG_TRACE;
  mmx::ConsoleTerminal::instance()->stop_wait();
}