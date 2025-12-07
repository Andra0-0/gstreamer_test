#include "console_terminal.h"

namespace mmx {

ConsoleTerminalPtr ConsoleTerminal::instance()
{
  if (!instance_) {
    std::lock_guard<mutex> _l(ins_lock_);
    if (instance_) return instance_;

    instance_ = std::make_shared<ConsoleTerminal>();
  }
  return instance_;
}

int ConsoleTerminal::thread_loop()
{
  return 0;
}

} // namespace mmx

int main(int argc, char *argv[])
{

}