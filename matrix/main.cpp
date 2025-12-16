#include <iostream>
#include <csignal>

#include "media_matrix.h"
#include "imessage_thread_manager.h"
#include "debug.h"

void init_all_modules()
{
  mmx::IMessageThreadManager::instance();
  mmx::MediaMatrix::instance();
}

void exit_all_modules()
{
  mmx::IMessageThreadManager::instance()->stop();
  mmx::MediaMatrix::instance()->exit_pending();
}

static void handle_signal(int sigint) {
  exit_all_modules();
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

  init_all_modules();

  auto media_matrix = mmx::MediaMatrix::instance();
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