#include "media_input_impl_hdmi.h"

#include <sstream>

#include "debug.h"

namespace mmx {

MediaInputImplHdmi::MediaInputImplHdmi()
{
  ALOG_TRACE;
}

MediaInputImplHdmi::~MediaInputImplHdmi()
{

}

gint MediaInputImplHdmi::init()
{
  ALOG_TRACE;
  gint ret = -1;

  do {
    ALOG_BREAK_IF(uri_.empty());

    source_ = gst_element_factory_make("v4l2src", "InputV4L2Source");
    ALOG_BREAK_IF(!source_);

    convert_ = gst_element_factory_make("videoconvert", "InputV4L2Convert");
    ALOG_BREAK_IF(!convert_);

    capsfilter_ = gst_element_factory_make("capsfilter", "InputV4L2Caps");
    ALOG_BREAK_IF(!capsfilter_);

    // input_selector_ = gst_element_factory_make("input-selector", "InputV4L2Selector");
    // ALOG_BREAK_IF(!input_selector_);

    tee_ = gst_element_factory_make("tee", "InputV4L2Tee");
    ALOG_BREAK_IF(!tee_);

    // queue_ = gst_element_factory_make("queue", "InputV4L2Queue");
    // ALOG_BREAK_IF(!queue_);

    g_object_set(G_OBJECT(source_),
            "device", uri_.c_str(),
            "io-mode", 4, // 4-DMABUF
            NULL);
    // g_object_set(G_OBJECT(queue_),
    //         "max-size-buffers", 50,
    //         "max-size-bytes", 0,
    //         "max-size-time", 0,
    //         "leaky", 2,
    //         NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            // "width", G_TYPE_INT, 1920, //width_,
            // "height", G_TYPE_INT, 1080,// height_,
            // "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(capsfilter_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    /* Link with capsfilter to constrain upstream negotiation: v4l2src -> capsfilter -> queue -> videoconvert */
    gst_bin_add_many(GST_BIN(bin_), source_, convert_, capsfilter_, tee_, NULL);
    if (!gst_element_link_many(source_, convert_, capsfilter_, tee_, NULL)) {
      ALOGD("Failed to link v4l2src -> capsfilter -> queue -> videoconvert");
    }

    // if (!prober_) {
    //   GstPad *pad = gst_element_get_static_pad(source_, "src");
    //   prober_ = std::make_shared<IPadProbeVideoInfo>(name_, pad);
    //   gst_object_unref(pad);
    // }
    state_ = kStreamStateReady;
    ret = 0;
  } while(0);

  // MediaInputModule::instance()->on_indev_video_pad_added(this);
  signal_indev_video_pad_added(this);
  // MediaInputModule::instance()->on_indev_no_more_pads(this);

  return ret;
}

gint MediaInputImplHdmi::deinit()
{
  do {
    state_ = kStreamStateInvalid;

    if (source_) {
      gst_element_set_state(source_, GST_STATE_NULL);
    }
  } while(0);

  return 0;
}

gint MediaInputImplHdmi::start()
{
  GstStateChangeReturn ret;

  do {
    state_ = kStreamStatePlaying;

    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PLAYING);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

    ret = gst_element_set_state(GST_ELEMENT(source_), GST_STATE_PLAYING);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);
  } while(0);

  return ret;
}

gint MediaInputImplHdmi::pause()
{
  GstStateChangeReturn ret;

  do {
    ALOG_BREAK_IF(state_ != kStreamStatePlaying);
    state_ = kStreamStatePause;

    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PAUSED);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);
  } while(0);

  return ret;
}

string MediaInputImplHdmi::get_info()
{
  std::ostringstream oss;
  GstState state, pending;

  oss << "\n\t============ get_info ============\n"
      << "\tClass: MediaInputImplImage\n"
      << "\tID: " << id_ << "\n"
      << "\tName: " << name_ << "\n";

  if (bin_) {
    gst_element_get_state(bin_, &state, &pending, 0);
    oss << "\tBinState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }
  if (source_) {
    gst_element_get_state(source_, &state, &pending, 0);
    oss << "\tV4L2srcState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }

  oss << "\t==================================";

  return oss.str();
}

GstPad* MediaInputImplHdmi::get_request_pad(const ReqPadInfo &info)
{
  GstPad *pad = nullptr;

  do {
    std::lock_guard<mutex> _l(lock_);
    if (info.is_video_) {
      pad = create_video_src_pad();
      // if (!prober_) {
      //   prober_ = std::make_shared<IPadProbeVideoInfo>(name_, pad);
      // }
    } else {
      // TODO
    }
  } while(0);

  return pad;
}

gint MediaInputImplHdmi::set_property(const IMessagePtr &msg)
{
  return 0;
}

void MediaInputImplHdmi::handle_bus_msg_error(GstBus *bus, GstMessage *msg)
{
  ALOG_TRACE;
  do {
    state_ = kStreamStateInvalid;
    gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PAUSED);
  } while(0);
}

// GstPad* MediaInputImplHdmi::create_video_src_pad()
// {
//   GstElement *new_queue;
//   GstPad *new_pad = nullptr;

//   do {
//     ALOG_BREAK_IF(state_ == kStreamStateInvalid);

//     new_queue = gst_element_factory_make("queue", nullptr);
//     ALOG_BREAK_IF(!new_queue);
//     gst_bin_add(GST_BIN(bin_), new_queue);

//     g_object_set(G_OBJECT(new_queue),
//             "max-size-buffers", 3,
//             "max-size-bytes", 0,
//             "max-size-time", 0,
//             "leaky", 2, // 0-no 1-upstream 2-downstream
//             NULL);

//     GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
//     GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
//     if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
//       ALOGD("Failed to link tee src pad to queue sink pad");
//     }
//     gst_object_unref(tee_src_pad);
//     gst_object_unref(queue_sink_pad);

//     GstPad *queue_src_pad = gst_element_get_static_pad(new_queue, "src");
//     if (queue_src_pad) {
//       new_pad = vpad_mngr_->add_pad(queue_src_pad);
//       gst_object_unref(queue_src_pad);
//     } else {
//       ALOGD("Failed to get src pad");
//     }

//     gst_element_sync_state_with_parent(new_queue);
//     ALOGD("Create video src pad success: %s", GST_PAD_NAME(new_pad));
//   } while(0);

//   return new_pad;
// }

GstPad* MediaInputImplHdmi::create_video_src_pad()
{
  GstElement *new_queue;
  GstElement *new_scale;
  GstElement *new_rate;
  GstElement *new_caps;
  GstPad *new_pad = nullptr;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_queue = gst_element_factory_make("queue", nullptr);
    ALOG_BREAK_IF(!new_queue);

    new_scale = gst_element_factory_make("videoscale", nullptr);
    ALOG_BREAK_IF(!new_scale);

    new_rate = gst_element_factory_make("videorate", nullptr);
    ALOG_BREAK_IF(!new_rate);

    new_caps = gst_element_factory_make("capsfilter", nullptr);
    ALOG_BREAK_IF(!new_caps);

    gst_bin_add_many(GST_BIN(bin_), new_queue, new_scale, new_rate, new_caps, NULL);
    if (!gst_element_link_many(new_queue, new_scale, new_rate, new_caps, NULL)) {
      ALOGD("Failed to link queue -> new_scale -> new_rate -> new_caps");
      // return nullptr;
    }

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            // "format", G_TYPE_STRING, "RGBA",
            // "width", G_TYPE_INT, 1920,
            // "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 30, 1, // 30fps
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(new_caps), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }
    g_object_set(G_OBJECT(new_queue),
            "max-size-buffers", 3,
            // "max-size-bytes", 0,
            // "max-size-time", 0,
            "leaky", 2, // 0-no 1-upstream 2-downstream
            NULL);

    GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
    GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link tee src pad to queue sink pad");
    }
    gst_object_unref(tee_src_pad);
    gst_object_unref(queue_sink_pad);

    GstPad *caps_src_pad = gst_element_get_static_pad(new_caps, "src");
    if (caps_src_pad) {
      new_pad = vpad_mngr_->add_pad(caps_src_pad);
      gst_object_unref(caps_src_pad);
    } else {
      ALOGD("Failed to get src pad");
    }
    if (!prober_) {
      GstPad *queue_srcpad = gst_element_get_static_pad(new_queue, "src");
      prober_ = std::make_shared<IPadProbeVideoInfo>(name_, queue_srcpad);
      gst_object_unref(queue_srcpad);
    }

    gst_element_sync_state_with_parent(new_queue);
    ALOGD("Create video src pad success: %s", GST_PAD_NAME(new_pad));
  } while(0);

  return new_pad;
}

}