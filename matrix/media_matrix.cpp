#include "media_matrix.h"

#include <cstddef>
#include <stddef.h>
#include <vector>

#include "debug.h"
// #include "input_impl_v4l2.h"
// #include "input_impl_file.h"
#include "media_input_impl_hdmi.h"
#include "media_input_module.h"
#include "output_impl_kms.h"

namespace mmx {

std::mutex MediaMatrix::inslock_;
std::shared_ptr<MediaMatrix> MediaMatrix::instance_ = nullptr;

MediaMatrix::MediaMatrix()
  : exit_pending_(false)
{
  // video convert 使用RGA加速
  g_setenv("GST_DEBUG_CONVERT_USE_RGA", "1", TRUE);
  g_setenv("GST_VIDEO_CONVERT_ONLY_RGA3", "1", TRUE);

  // debug 环境变量
  // g_setenv("GST_DEBUG","3", TRUE);
  g_setenv("GST_DEBUG","3,rgavideoconvert:5", TRUE);
  // g_setenv("GST_DEBUG_FILE","/tmp/mmdebug.log", TRUE);
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", MEDIA_MATRIX_DOT_DIR, TRUE);

  /* Initialize GStreamer */
  gst_init(NULL, NULL);

  /* Create the empty pipeline */
  pipeline_ = gst_pipeline_new ("matrix-pipeline");
  bus_ = gst_element_get_bus (pipeline_);

  // gst_bus_add_signal_watch(bus_);
  // g_signal_connect(G_OBJECT(bus_), "message::error", (GCallback)on_handle_bus_msg_error, this);
  // g_signal_connect(G_OBJECT(bus_), "message::eos", (GCallback)on_handle_bus_msg_eos, this);
}

MediaMatrix::~MediaMatrix()
{

}

gint MediaMatrix::init(int argc, char *argv[])
{
  gint ret = -1;

  do {
    /* Create compositor and sink */
    // mix_compositor_ = gst_element_factory_make ("compositor", "video-compositor");
    // ALOG_BREAK_IF(!mix_compositor_);

    // sink_ = gst_element_factory_make ("kmssink", "video-sink");
    // ALOG_BREAK_IF(!sink_);

    /* Create input objects and their bins */
    // InputIntfPtr v4l2in = std::make_shared<InputImplV4L2>("/dev/video0", 4);
    // InputIntfPtr v4l2in = InputIntfManager::instance()->create(INPUT_TYPE_V4L2);
    // MediaInputIntfPtr v4l2in = MediaInputModule::instance()->create(VideoInputType::kVideoInputHdmi);
    MediaInputIntfPtr v4l2in;
    v4l2in = MediaInputModule::instance()->create(VideoInputType::kVideoInputUridb);
    ALOGD("%s", v4l2in->name());
    // InputIntfPtr filein = std::make_shared<InputImplFile>("/home/cat/test/sample_2560x1440.mp4");

    OutputIntfPtr kmsout = std::make_shared<OutputImplKms>(OutputImplKms::HDMI_TX_0);

    inputs_.push_back(v4l2in);
    /* Keep the output object alive by storing it in outputs_ vector */
    outputs_.push_back(kmsout);
    // inputs_.push_back(filein);

    /* Build the pipeline: add input bins, compositor and sink */
    GstElement *v4l2_bin = MediaInputModule::instance()->get_bin();
    // GstElement *file_bin = filein->bin();
    GstElement *sink_bin = kmsout->bin();

    gst_bin_add_many(GST_BIN(pipeline_), v4l2_bin, /*file_bin, mix_compositor_,*/ sink_bin, NULL);

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
    // GstPad *src_pad = v4l2in->get_src_pad("default");
    GstPad *src_pad = MediaInputModule::instance()->get_request_pad(v4l2in);
    if (src_pad == NULL) ALOGD("ERROR");
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
    // bus_ = gst_element_get_bus (pipeline_);

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

  if (bus_ != nullptr) {
    gst_object_unref(bus_);
    bus_ = nullptr;
  }
  if (pipeline_ != nullptr) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
  inputs_.clear();
  outputs_.clear();
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

    // GError *err = nullptr;
    // gchar *debug_info = nullptr;

    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        // gst_message_parse_error(msg, &err, &debug_info);
        // ALOGD("Error received from element %s: %s",
        //     GST_OBJECT_NAME (msg->src), err ? err->message : "(null)");
        // ALOGD("Debugging information: %s",
        //     debug_info ? debug_info : "none");
        // g_clear_error (&err);
        // g_free (debug_info);
        on_handle_bus_msg_error(bus_, msg, nullptr);
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

  deinit();
  return 0;
}

gboolean MediaMatrix::on_handle_bus_msg_error(GstBus *bus, GstMessage *msg, void *data)
{
  gboolean ret = TRUE;
  GError *err = nullptr;
  gchar *debug_info = nullptr;

  do {
    gst_message_parse_error(msg, &err, &debug_info);
    ALOGD("BusError:%s from element:%s", err ? err->message : "(null)", GST_OBJECT_NAME (msg->src));
    ALOGD("BusDebug:%s", debug_info ? debug_info : "none");

    GstObject *src_obj = msg->src;
    GstElement *videoin_bin = MediaInputModule::instance()->get_bin();
    if (src_obj && videoin_bin && gst_object_has_ancestor(src_obj, GST_OBJECT(videoin_bin))) {
      ALOGD("Error comes from MediaInputModule subtree; trying to locate failing input");
      MediaMatrix *self = MediaMatrix::instance().get();
      for (size_t i = 0; i < self->inputs_.size(); ++i) {
        MediaInputIntfPtr input = self->inputs_[i];
        if (!input) continue;
        GstElement *inbin = input->get_bin();
        if (!inbin) continue;
        if (gst_object_has_ancestor(src_obj, GST_OBJECT(inbin))) {
          ALOGD("Detected error in input %s (id=%d)", input->name(), input->id());

          ALOGD("Attempting soft-restart of input bin %s", input->name());
          gst_element_set_state(inbin, GST_STATE_READY);
          gst_element_set_state(inbin, GST_STATE_PLAYING);

          if (GST_IS_ELEMENT(msg->src)) {
            GstElement *e = GST_ELEMENT(msg->src);
            GstElementFactory *factory = gst_element_get_factory(e);
            if (factory) {
              const gchar *fname = gst_plugin_feature_get_name(factory);
              if (fname && g_str_has_prefix(fname, "uridecodebin")) {
                ALOGD("Error originated from uridecodebin (or child). Consider checking the URI, parser chain and caps negotiation.");
                // TODO
              }
              gst_object_unref(factory);
            }
          }
          break;
        }
      }
    }
  } while(0);

  return ret;
}

gboolean MediaMatrix::on_handle_bus_msg_eos(GstBus *bus, GstMessage *msg, void *data)
{
  return TRUE;
}

} // namespace mmx