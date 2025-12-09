#ifndef MATRIX_CONSOLE_INTERFACE_H
#define MATRIX_CONSOLE_INTERFACE_H

#include <memory>
#include <mutex>

#include "ithread.h"

namespace mmx {

class ConsoleInterface;

using std::shared_ptr;
using std::mutex;
using ConsoleInterfacePtr = shared_ptr<ConsoleInterface>;

class ConsoleInterface : public IThread {
public:
  static inline ConsoleInterfacePtr instance();

protected:
  virtual int thread_loop() override;

private:
  int thread_init();

  int thread_deinit();

private:
  static ConsoleInterfacePtr instance_;
  static mutex ins_lock_;
};

} // namespace mmx

#endif // MATRIX_CONSOLE_INTERFACE_H