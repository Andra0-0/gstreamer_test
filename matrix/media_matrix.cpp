#include "media_matrix.h"

#include <cstddef>
#include <stddef.h>
#include <vector>

#include "debug.h"
#include "input_impl_v4l2.h"
#include "input_impl_file.h"
#include "output_impl_kms.h"

std::mutex MediaMatrix::inslock_;
std::shared_ptr<MediaMatrix> MediaMatrix::instance_ = nullptr;

MediaMatrix::MediaMatrix()
  : exit_pending_(false)
{
  // video convert 使用RGA加速
  g_setenv("GST_DEBUG_CONVERT_USE_RGA", "1", TRUE);
  g_setenv("GST_VIDEO_CONVERT_ONLY_RGA3", "1", TRUE);

  // debug 环境变量
  g_setenv("GST_DEBUG","3", TRUE);
  // g_setenv("GST_DEBUG_FILE","/tmp/mmdebug.log", TRUE);
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", MEDIA_MATRIX_DOT_DIR, TRUE);
}

MediaMatrix::~MediaMatrix()
{
  if (bus_ != nullptr) {
    gst_object_unref(bus_);
    bus_ = nullptr;
  }
  if (pipeline_ != nullptr) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
}

gint MediaMatrix::init(int argc, char *argv[])
{
  gint ret = -1;

  do {
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the empty pipeline */
    pipeline_ = gst_pipeline_new ("matrix-pipeline");
    /* Create compositor and sink */
    mix_compositor_ = gst_element_factory_make ("compositor", "video-compositor");
    ALOG_BREAK_IF(!mix_compositor_);

    // sink_ = gst_element_factory_make ("kmssink", "video-sink");
    // ALOG_BREAK_IF(!sink_);

    /* Create input objects and their bins */
    InputIntfPtr v4l2in = std::make_shared<InputImplV4L2>("/dev/video0", 4);
    // InputIntfPtr filein = std::make_shared<InputImplFile>("/home/cat/test/sample_2560x1440.mp4");

    OutputIntfPtr kmsout = std::make_shared<OutputImplKms>(OutputImplKms::HDMI_TX_0);

    inputs_.push_back(v4l2in);
    /* Keep the output object alive by storing it in outputs_ vector */
    outputs_.push_back(kmsout);
    // inputs_.push_back(filein);

    /* Modify properties */
    g_object_set(G_OBJECT(mix_compositor_), // compositor
            "background", 1, // black background
            NULL);
    // g_object_set (G_OBJECT(sink_), // kmssink
    //         "sync", FALSE,
    //         "skip-vsync", TRUE,
    //         NULL);

    /* Build the pipeline: add input bins, compositor and sink */
    GstElement *v4l2_bin = v4l2in->bin();
    // GstElement *file_bin = filein->bin();
    GstElement *sink_bin = kmsout->bin();

    gst_bin_add_many(GST_BIN(pipeline_), v4l2_bin, /*file_bin,*/ mix_compositor_, sink_bin, NULL);

    // if (gst_element_link_many(mix_compositor_, sink_bin, NULL) != TRUE) {
    //   ALOGD("Elements could not be linked: mix_compositor_ -> sink_");
    //   gst_object_unref (pipeline_);
    //   break;
    // }
    // GstPad *compo_src_pad = gst_element_get_static_pad(mix_compositor_, "src");
    // GstPad *sink_sink_pad = kmsout->sink_pad();
    // if (gst_pad_link(compo_src_pad, sink_sink_pad) != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link compositor src pad to sink sink pad");
    //   gst_object_unref(compo_src_pad);
    //   gst_object_unref(sink_sink_pad);
    //   break;
    // }
    GstPad *src_pad = v4l2in->src_pad();
    GstPad *sink_pad = kmsout->sink_pad();
    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link compositor src pad to sink sink pad");
      gst_object_unref(src_pad);
      gst_object_unref(sink_pad);
      break;
    }

    /* Setup compositor pads and link inputs */
    // if (setupCompositorPads() != 0) {
    //   ALOGD("Failed to setup compositor pads");
    //   gst_object_unref(pipeline_);
    //   break;
    // }

    /* Start playing */
    ret = gst_element_set_state (pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      ALOGD("Unable to set the pipeline to the playing state.");
      gst_object_unref (pipeline_);
      return -1;
    }
    bus_ = gst_element_get_bus (pipeline_);

    /* 在这里生成DOT文件 */
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_),
        GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-init");
    ret = 0;
  } while(0);

  return ret;
}

gint MediaMatrix::deinit()
{
  /* 在结束前再生成一个DOT文件，显示可能的状态变化 */
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_),
      GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-deinit");

  exit_pending_ = true;
  return 0;
}

gint MediaMatrix::join()
{
  GstMessage *msg = nullptr;

  while (!exit_pending_) {
    msg = gst_bus_timed_pop_filtered (bus_, /*timeout*/ GST_MSECOND * 500,
        (GstMessageType)(GST_MESSAGE_ERROR|GST_MESSAGE_EOS));
    if (exit_pending_) {
      break;
    }
    if (msg == NULL) {
      continue;
    }

    GError *err = nullptr;
    gchar *debug_info = nullptr;

    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        ALOGD("Error received from element %s: %s",
            GST_OBJECT_NAME (msg->src), err ? err->message : "(null)");
        ALOGD("Debugging information: %s",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        gst_message_unref (msg);
        return 0;
      case GST_MESSAGE_EOS:
        ALOGD("End-Of-Stream reached.");
        gst_message_unref (msg);
        return 0;
      default:
        ALOGD("Unexpected message received.");
        gst_message_unref (msg);
        return 0;
    }
  }

  return 0;
}

gint MediaMatrix::setupCompositorPads()
{
  if (inputs_.empty()) return -1;

  for (uint64_t i = 0; i < inputs_.size(); ++i) {
    GstPad *sink_pad = gst_element_get_request_pad(mix_compositor_, "sink_%u");
    if (!sink_pad) {
      ALOGD("Failed to get request pad from compositor");
      return -1;
    }

    // Simple layout: two columns side-by-side for first two streams
    if (i == 0) {
      g_object_set(sink_pad, "xpos", 0, "ypos", 0, "width", 1920, "height", 1080, NULL);
    } else if (i == 1) {
      g_object_set(sink_pad, "xpos", 960, "ypos", 0, "width", 960, "height", 1080, NULL);
    } else {
      // fallback place subsequent streams at 0,0 with small size
      g_object_set(sink_pad, "xpos", 0, "ypos", 0, "width", 320, "height", 240, NULL);
    }

    GstPad *src_pad = inputs_[i]->src_pad();
    if (!src_pad) {
      ALOGD("Failed to get src pad from input %zu", i);
      gst_object_unref(sink_pad);
      return -1;
    }

    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link input %zu to compositor sink", i);
      gst_object_unref(sink_pad);
      gst_object_unref(src_pad);
      return -1;
    }

    gst_object_unref(sink_pad);
    gst_object_unref(src_pad);
  }

  return 0;
}