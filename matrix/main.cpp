#include <iostream>
#include <csignal>

#include "media_matrix.h"
#include "imessage_thread_manager.h"
#include "media_input_module.h"
#include "media_define.h"
#include "debug.h"

void inst_all_modules()
{
  mmx::IMessageThreadManager::instance();
  mmx::MediaInputModule::instance();
  mmx::MediaMatrix::instance();
}

void exit_all_modules()
{
  mmx::IMessageThreadManager::instance()->stop();
  mmx::MediaInputModule::instance()->stop();
  mmx::MediaMatrix::instance()->stop();
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

  // 开启RGA日志
  // g_setenv("ROCKCHIP_RGA_LOG", "1", TRUE);

  // video convert 使用RGA加速
  g_setenv("GST_DEBUG_CONVERT_USE_RGA", "1", TRUE);
  g_setenv("GST_VIDEO_CONVERT_ONLY_RGA3", "1", TRUE);

  // Gst 系统时钟
  g_setenv("GST_CLOCK_TYPE", "system", TRUE);
  // g_setenv("GST_USE_SYSTEM_CLOCK", "1", TRUE);

  // debug 环境变量
  // g_setenv("GST_DEBUG","3", TRUE);
  // g_setenv("GST_DEBUG","1,rgavideoconvert:5", TRUE);
  g_setenv("GST_DEBUG","1,compositor:5,rgacompositor:5,rgacompositor_rkrga:5", TRUE);
  // g_setenv("GST_DEBUG","1,videoaggregator:5", TRUE);
  // g_setenv("GST_DEBUG_FILE","/tmp/mmdebug.log", TRUE);
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", MEDIA_MATRIX_DOT_DIR, TRUE);

  /* Initialize GStreamer */
  gst_init(NULL, NULL);

  // bus_msg_handler = thread(std::bind(&MediaMatrix::handle_bus_msg, this));

  // gst_bus_add_signal_watch(bus_);
  // g_signal_connect(G_OBJECT(bus_), "message::error", (GCallback)on_handle_bus_msg_error, this);
  // g_signal_connect(G_OBJECT(bus_), "message::eos", (GCallback)on_handle_bus_msg_eos, this);

  inst_all_modules();

  auto media_matrix = mmx::MediaMatrix::instance();

  do {
    ret = media_matrix->init(argc, argv);
    if (ret != 0) {
      ALOGD("MediaMatrix init failed.");
      break;
    }
    // media_matrix->handle_bus_msg();
    media_matrix->join();
  } while (0);

  media_matrix->deinit();
  return ret;
}