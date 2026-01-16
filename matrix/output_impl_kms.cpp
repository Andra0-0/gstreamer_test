#include "output_impl_kms.h"

#include <drm/drm.h>
#include <fcntl.h>
#include <unistd.h>

#include "debug.h"

namespace mmx {

OutputImplKms::DRM::DRM(const char *device)
  : fd_(-1)
  , device_(device)
  , res_(nullptr)
  , plane_res_(nullptr)
{

}

OutputImplKms::DRM::~DRM()
{
  deinit();
}

bool OutputImplKms::DRM::init()
{
  fd_ = open(device_.c_str(), O_RDWR | O_CLOEXEC);
  if (fd_ < 0) {
    ALOGD("Failed to open DRM device %s", device_);
    return false;
  }

  res_ = drmModeGetResources(fd_);
  if (!res_) {
    ALOGD("Failed to get DRM resources");
    ::close(fd_);
    fd_ = -1;
    return false;
  }
  for (int i = 0; i < res_->count_connectors; ++i) {
    printf("connector[%d] = %d", i, res_->connectors[i]);
  }
  drmSetClientCap(fd_, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  plane_res_ = drmModeGetPlaneResources(fd_);
  if (!plane_res_) {
    ALOGD("Failed to get DRM plane resources");
    drmModeFreeResources(res_);
    drmClose(fd_);
    fd_ = -1;
    return false;
  }
  for (int i = 0; i < plane_res_->count_planes; i++) {
    printf("plane[%d] = %d\n", i, plane_res_->planes[i]);
  }
  return true;
}

void OutputImplKms::DRM::deinit()
{
  if (plane_res_) {
    drmModeFreePlaneResources(plane_res_);
    plane_res_ = nullptr;
  }
  if (res_) {
    drmModeFreeResources(res_);
    res_ = nullptr;
  }
  if (fd_ >= 0) {
    drmClose(fd_);
    fd_ = -1;
  }
}

OutputImplKms::OutputImplKms(hdmi_tx_port port)
  : bin_(nullptr)
  , kms_sink_(nullptr)
{
  do {
    bin_ = gst_bin_new("OutputKmsBin");
    ALOG_BREAK_IF(!bin_);

    kms_sink_ = gst_element_factory_make("kmssink", "OutputKmsSink");
    // kms_sink_ = gst_element_factory_make("fpsdisplaysink", "OutputKmsSink");
    ALOG_BREAK_IF(!kms_sink_);

    tee_ = gst_element_factory_make("tee", "OutputKmsTee");
    ALOG_BREAK_IF(!tee_);

    queue_ = gst_element_factory_make("queue", "OutputKmsQueue");
    ALOG_BREAK_IF(!queue_);

    convert_ = gst_element_factory_make("videoconvert", "OutputKmsConvert");
    // convert_ = gst_element_factory_make("rgavideoconvert", "OutputKmsConvert");
    ALOG_BREAK_IF(!convert_);

    capsfilter_ = gst_element_factory_make("capsfilter", "OutputKmsCaps");
    ALOG_BREAK_IF(!capsfilter_);

    p_drm_ = std::make_unique<DRM>("/dev/dri/card0");
    ALOG_BREAK_IF(!p_drm_ || !p_drm_->init());

    g_object_set(G_OBJECT(kms_sink_),
            "fd", p_drm_->fd_,
            "connector-id", p_drm_->res_->connectors[port],
            // "plane-id", p_drm_->plane_res_->planes[port+2],
            // "async", FALSE,
            "sync", FALSE,
            "skip-vsync", TRUE,
            NULL);
    g_object_set(G_OBJECT(queue_),
            "max-size-buffers", 5,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 2, // 0-no 1-upstream 2-downstream
            NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(capsfilter_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    gst_bin_add_many(GST_BIN(bin_), tee_, queue_, convert_, capsfilter_, kms_sink_, NULL);
    if (!gst_element_link_many(tee_, queue_, convert_, capsfilter_, kms_sink_, NULL)) {
      ALOGD("Failed to link tee -> queue -> convert_ -> capsfilter -> kmssink");
    }

    // 创建ghost pad作为这个bin的输入
    GstPad *pad = gst_element_get_static_pad(tee_, "sink");
    if (pad) {
      // if (!prober_) {
      //   prober_ = std::make_shared<mmx::IPadProbeVideoInfo>("kmssink", pad);
      // }
      GstPad *ghost = gst_ghost_pad_new("sink", pad);
      gst_element_add_pad(bin_, ghost);
      gst_pad_set_active(ghost, TRUE);
      gst_object_unref(pad);
    } else {
      ALOGD("Failed to get tee sink pad");
    }
  } while(0);
}

OutputImplKms::~OutputImplKms()
{
  // if (bin_) {
  //   gst_object_unref(bin_);
  //   bin_ = nullptr;
  // }
}

GstPad* OutputImplKms::sink_pad()
{
  if (!bin_) return nullptr;
  return gst_element_get_static_pad(bin_, "sink");
}

gint OutputImplKms::refresh()
{
  if (queue_) {
    gst_element_send_event(queue_, gst_event_new_flush_start());
    gst_element_send_event(queue_, gst_event_new_flush_stop(TRUE));
  }
  return 0;
}

}