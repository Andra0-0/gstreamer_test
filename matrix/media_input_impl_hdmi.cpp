#include "media_input_impl_hdmi.h"

#include <sstream>

#include "media_input_module.h"
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

    bin_ = gst_bin_new(name_.c_str());
    ALOG_BREAK_IF(!bin_);

    source_ = gst_element_factory_make("v4l2src", "InputV4L2Source");
    ALOG_BREAK_IF(!source_);

    convert_ = gst_element_factory_make("videoconvert", "InputV4L2Convert");
    ALOG_BREAK_IF(!convert_);

    capsfilter_ = gst_element_factory_make("capsfilter", "InputV4L2Caps");
    ALOG_BREAK_IF(!capsfilter_);

    input_selector_ = gst_element_factory_make("input-selector", "InputV4L2Selector");
    ALOG_BREAK_IF(!input_selector_);

    tee_ = gst_element_factory_make("tee", "InputV4L2Tee");
    ALOG_BREAK_IF(!tee_);

    // queue_ = gst_element_factory_make("queue", "InputV4L2Queue");
    // ALOG_BREAK_IF(!queue_);

    pad_manager_ = std::make_shared<IGhostPadManager>(bin_, "video_src");

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
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, 1920, //width_,
            "height", G_TYPE_INT, 1080,// height_,
            "framerate", GST_TYPE_FRACTION, 30, 1,
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

    state_ = kStreamStateReady;
    ret = 0;
  } while(0);

  MediaInputModule::instance()->on_indev_video_pad_added(this);
  // MediaInputModule::instance()->on_indev_no_more_pads(this);

  return ret;
}

gint MediaInputImplHdmi::deinit()
{
  do {
    state_ = kStreamStateInvalid;
  } while(0);

  return 0;
}

gint MediaInputImplHdmi::start()
{
  do {
    state_ = kStreamStatePlaying;
  } while(0);

  return 0;
}

gint MediaInputImplHdmi::pause()
{
  do {
    state_ = kStreamStatePause;
  } while(0);

  return 0;
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

GstElement* MediaInputImplHdmi::get_bin()
{
  return bin_;
}

GstPad* MediaInputImplHdmi::get_request_pad(bool is_video)
{
  GstPad *pad = nullptr;

  do {
    std::lock_guard<mutex> _l(lock_);
    if (is_video) {
      pad = create_video_src_pad();
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

GstPad* MediaInputImplHdmi::create_video_src_pad()
{
  GstElement *new_queue;
  GstPad *new_pad = nullptr;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_queue = gst_element_factory_make("queue", nullptr);
    ALOG_BREAK_IF(!new_queue);
    gst_bin_add(GST_BIN(bin_), new_queue);

    g_object_set(G_OBJECT(new_queue),
            "max-size-buffers", 3,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 2, // 0-no 1-upstream 2-downstream
            NULL);

    GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
    GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link tee src pad to queue sink pad");
    }
    gst_object_unref(tee_src_pad);
    gst_object_unref(queue_sink_pad);

    GstPad *queue_src_pad = gst_element_get_static_pad(new_queue, "src");
    if (queue_src_pad) {
      new_pad = pad_manager_->add_pad(queue_src_pad);
      gst_object_unref(queue_src_pad);
    } else {
      ALOGD("Failed to get src pad");
    }

    gst_element_sync_state_with_parent(new_queue);
    ALOGD("Create video src pad success: %s", GST_PAD_NAME(new_pad));
  } while(0);

  return new_pad;
}

}