#include "output_impl_kms.h"

#include <drm/drm.h>
#include <fcntl.h>
#include <unistd.h>

#include "debug.h"

OutputImplKms::DRM::DRM(const char *device)
{
  fd = open(device, O_RDWR | O_CLOEXEC);
  if (fd < 0) {
    ALOGD("Failed to open DRM device %s", device);
    return;
  }

  res = drmModeGetResources(fd);
  if (!res) {
    ALOGD("Failed to get DRM resources");
    ::close(fd);
    fd = -1;
    return;
  }
  for (int i = 0; i < res->count_connectors; ++i) {
    printf("connector[%d] = %d", i, res->connectors[i]);
  }
  drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

  plane_res = drmModeGetPlaneResources(fd);
  if (!plane_res) {
    ALOGD("Failed to get DRM plane resources");
    drmModeFreeResources(res);
    drmClose(fd);
    fd = -1;
    return;
  }
  for (int i = 0; i < plane_res->count_planes; i++) {
    printf("plane[%d] = %d\n", i, plane_res->planes[i]);
  }
}

OutputImplKms::DRM::~DRM()
{
  if (plane_res) {
    drmModeFreePlaneResources(plane_res);
    plane_res = nullptr;
  }
  if (res) {
    drmModeFreeResources(res);
    res = nullptr;
  }
  if (fd >= 0) {
    drmClose(fd);
    fd = -1;
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
    ALOG_BREAK_IF(!kms_sink_);

    tee_ = gst_element_factory_make("tee", "OutputKmsTee");
    ALOG_BREAK_IF(!tee_);

    queue_ = gst_element_factory_make("queue", "OutputKmsQueue");
    ALOG_BREAK_IF(!queue_);

    convert_ = gst_element_factory_make("videoconvert", "OutputKmsConvert");
    /*Debug*/
    // convert_ = gst_element_factory_make("rgavideoconvert", "OutputKmsConvert");
    ALOG_BREAK_IF(!convert_);

    capsfilter_ = gst_element_factory_make("capsfilter", "OutputKmsCaps");
    ALOG_BREAK_IF(!capsfilter_);

    // FIXME: core dump here
    // p_drm_ = std::make_unique<DRM>("/dev/dri/card0");
    // ALOG_BREAK_IF(!p_drm_ || p_drm_->fd < 0);

    g_object_set(G_OBJECT(kms_sink_),
            // "fd", p_drm_->fd,
            // "connector-id", p_drm_->res->connectors[port],
            // "plane-id", p_drm_->plane_res->planes[port+2],
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
            "format", G_TYPE_STRING, "RGBA",
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
      //   prober_ = std::make_shared<mmx::IPadProber>(
      //           pad, GST_PAD_PROBE_TYPE_BUFFER, mmx::deffunc_videoframe_info);
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