#ifndef MATRIX_CONSOLE_TERMINAL_H
#define MATRIX_CONSOLE_TERMINAL_H

#include <memory>
#include <mutex>

#include "ithread.h"

namespace mmx {

class ConsoleTerminal;

using std::shared_ptr;
using std::mutex;
using ConsoleTerminalPtr = shared_ptr<ConsoleTerminal>;

class ConsoleTerminal : public IThread {
public:
  static inline ConsoleTerminalPtr instance();

protected:
  virtual int thread_loop() override;

private:
  static ConsoleTerminalPtr instance_;
  static mutex ins_lock_;
};

} // namespace mmx

#endif // MATRIX_CONSOLE_TERMINAL_H