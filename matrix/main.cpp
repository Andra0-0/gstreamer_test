#include <iostream>
#include <csignal>

#include "media_matrix.h"
#include "debug.h"

static void handle_signal(int /*sig*/) {
  /* Only set an atomic flag here — do not call non-async-signal-safe functions. */
  MediaMatrix::instance()->deinit();
}

int main(int argc, char *argv[]) {
  int ret = -1;

  //注册信号处理函数
  struct sigaction sa = {0};
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);  // CTRL+C
  sigaction(SIGTERM, &sa, NULL); // kill

  auto media_matrix = MediaMatrix::instance();
  ret = media_matrix->init(argc, argv);
  if (ret != 0) {
    ALOGD("MediaMatrix init failed.");
    return -1;
  }

  do {
    media_matrix->join();
    // ret = media_hub_run(argc, argv);
  } while (0);

  return ret;
}