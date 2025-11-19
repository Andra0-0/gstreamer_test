#include "input_impl_v4l2.h"

#include "debug.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

static GstPadProbeReturn pad_probe_log_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
  if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) {
    GstBuffer *gbuf = gst_pad_probe_info_get_buffer(info);
    if (gbuf) {
      ALOGD("[probe] %s src buffer PTS=%" GST_TIME_FORMAT,
            GST_ELEMENT_NAME(GST_PAD_PARENT(pad)), GST_TIME_ARGS(GST_BUFFER_PTS(gbuf)));
    }
  }
  return GST_PAD_PROBE_OK;
}

InputImplV4L2::InputImplV4L2(const std::string &device, int io_mode)
  : bin_(nullptr)
  , source_(nullptr)
  , convert_(nullptr)
{
  do {
    bin_ = gst_bin_new("InputV4L2Bin");
    ALOG_BREAK_IF(!bin_);

    source_ = gst_element_factory_make("v4l2src", "InputV4L2Source");
    ALOG_BREAK_IF(!source_);

    convert_ = gst_element_factory_make("videoconvert", "InputV4L2Convert");
    ALOG_BREAK_IF(!convert_);

    capsfilter_ = gst_element_factory_make("capsfilter", "InputV4L2Caps");
    ALOG_BREAK_IF(!capsfilter_);

    tee_ = gst_element_factory_make("tee", "InputV4L2Tee");
    ALOG_BREAK_IF(!tee_);

    queue_ = gst_element_factory_make("queue", "InputV4L2Queue");
    ALOG_BREAK_IF(!queue_);

    g_object_set(G_OBJECT(source_),
            "device", "/dev/video0",
            "io-mode", 4,
            NULL);
    g_object_set(G_OBJECT(queue_),
            "max-size-buffers", 50,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 2,
            NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "BGR",
            "width", G_TYPE_INT, 1920,
            "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(capsfilter_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    /* Link with capsfilter to constrain upstream negotiation: v4l2src -> capsfilter -> queue -> videoconvert */
    gst_bin_add_many(GST_BIN(bin_), source_, convert_, capsfilter_, tee_, queue_, NULL);
    if (!gst_element_link_many(source_, convert_, capsfilter_, tee_, NULL)) {
      ALOGD("Failed to link v4l2src -> capsfilter -> queue -> videoconvert");
    }

    GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
    GstPad *queue_sink_pad = gst_element_get_static_pad(queue_, "sink");
    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link tee src pad to queue sink pad");
    }
    gst_object_unref(tee_src_pad);
    gst_object_unref(queue_sink_pad);

    // 创建ghost pad作为这个bin的最终输出
    GstPad *pad = gst_element_get_static_pad(queue_, "src");
    if (pad) {
      GstPad *ghost = gst_ghost_pad_new("src", pad);
      gst_element_add_pad(bin_, ghost);
      gst_object_unref(pad);
    } else {
      ALOGD("Failed to get convert src pad");
    }

    /* also probe convert and source src pads to know where buffers stop */
    /* also probe source and queue src pads to know where buffers stop */
    // GstPad *pad_qsrc = gst_element_get_static_pad(queue_, "src");
    // if (pad_qsrc) {
    //   gst_pad_add_probe(pad_qsrc, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_log_cb, this, NULL);
    //   gst_object_unref(pad_qsrc);
    // }

    // GstPad *pad_s = gst_element_get_static_pad(source_, "src");
    // if (pad_s) {
    //   gst_pad_add_probe(pad_s, GST_PAD_PROBE_TYPE_BUFFER, pad_probe_log_cb, this, NULL);
    //   gst_object_unref(pad_s);
    // }
  } while(0);
}

InputImplV4L2::~InputImplV4L2()
{
  if (bin_) {
    gst_object_unref(bin_);
  }
}

GstPad* InputImplV4L2::src_pad()
{
  if (!bin_) {
    return nullptr;
  }
  return gst_element_get_static_pad(bin_, "src");
}