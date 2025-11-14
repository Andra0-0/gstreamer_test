#include "media_matrix.h"

#include "debug.h"

std::mutex MediaMatrix::inslock_;
std::shared_ptr<MediaMatrix> MediaMatrix::instance_ = nullptr;

MediaMatrix::MediaMatrix()
{
  g_setenv("GST_DEBUG","3", TRUE);
  g_setenv("GST_DEBUG_DUMP_DOT_DIR", MEDIA_MATRIX_DOT_DIR, TRUE);
}

MediaMatrix::~MediaMatrix()
{
  
}

gint MediaMatrix::init(int argc, char *argv[])
{
  gint ret = -1;

  do {
    /* Initialize GStreamer */
    gst_init(&argc, &argv);

    /* Create the elements */
    source_ = gst_element_factory_make ("videotestsrc", "source");
    filter_ = gst_element_factory_make ("vertigotv", "filter");
    sink_ = gst_element_factory_make ("autovideosink", "sink");

    /* Create the empty pipeline */
    pipeline_ = gst_pipeline_new ("test-pipeline");

    ALOG_BREAK_IF(!pipeline_ || !source_ || !sink_);

    /* Build the pipeline */
    gst_bin_add_many(GST_BIN(pipeline_), source_, filter_, sink_, NULL);
    if (gst_element_link_many(source_, filter_, sink_, NULL) != TRUE) {
      ALOGD("Elements could not be linked.");
      gst_object_unref (pipeline_);
      break;
    }

    /* Modify the source's properties */
    g_object_set (source_, "pattern", 0, NULL);

    /* Start playing */
    ret = gst_element_set_state (pipeline_, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      ALOGD("Unable to set the pipeline to the playing state.");
      gst_object_unref (pipeline_);
      return -1;
    }
    bus_ = gst_element_get_bus (pipeline_);

    /* 在这里生成DOT文件 */
    GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_),
        GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-init");
    ret = 0;
  } while(0);

  return ret;
}

gint MediaMatrix::deinit()
{
  /* 在结束前再生成一个DOT文件，显示可能的状态变化 */
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_),
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
  return 0;
}

gint MediaMatrix::join()
{
  GstMessage *msg = nullptr;
  msg = gst_bus_timed_pop_filtered (bus_, GST_CLOCK_TIME_NONE,
      (GstMessageType)(GST_MESSAGE_ERROR|GST_MESSAGE_EOS));

  if (msg != NULL) { /* Parse message */
    GError *err;
    gchar *debug_info;

    switch (GST_MESSAGE_TYPE(msg)) {
      case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        ALOGD("Error received from element %s: %s",
            GST_OBJECT_NAME (msg->src), err->message);
        ALOGD("Debugging information: %s",
            debug_info ? debug_info : "none");
        g_clear_error (&err);
        g_free (debug_info);
        break;
      case GST_MESSAGE_EOS:
        ALOGD("End-Of-Stream reached.");
        break;
      default:
        /* We should not reach here because we only asked for ERRORs and EOS */
        ALOGD("Unexpected message received.");
        break;
    }
    gst_message_unref (msg);
  }
  return 0;
}