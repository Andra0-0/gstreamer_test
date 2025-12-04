#include "ipad_prober.h"

#include <gst/video/video.h> // GstVideoMeta

#include "debug.h"

namespace mmx {

// GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM 下游事件
GstPadProbeReturn deffunc_handle_event(GstPadProbeInfo *info)
{
  GstEvent *event;
  GstCaps *caps;
  gchar *caps_str;

  do {
    event = GST_PAD_PROBE_INFO_EVENT(info);
    ALOG_BREAK_IF(!event);

    switch(GST_EVENT_TYPE(event)) {
      case GST_EVENT_CAPS:
        caps = nullptr;
        gst_event_parse_caps(event, &caps);
        ALOG_BREAK_IF(!caps);

        caps_str = gst_caps_to_string(caps);
        ALOGD("IPadProber recv event-caps: %s", caps_str);
        g_free(caps_str);

        gst_caps_unref(caps);
        break;
      case GST_EVENT_EOS:
        ALOGD("IPadProber recv event-eos");
        break;
      default:
        ALOGD("IPadProber recv event-unknown: %d", GST_EVENT_TYPE(event));
        break;
    }

    gst_event_unref(event);
  } while(0);

  return GST_PAD_PROBE_OK;
}

// GST_PAD_PROBE_TYPE_BUFFER 缓冲区（视频帧）
GstPadProbeReturn deffunc_videoframe_info(GstPadProbeInfo *info)
{
  GstBuffer *buffer;
  GstVideoMeta *video_meta;

  do {
    buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    ALOG_BREAK_IF(!buffer);

    video_meta = gst_buffer_get_video_meta(buffer);
    ALOG_BREAK_IF(!video_meta);

    ALOGD("vf fmt:%d w:%u h:%u pts:%lld",
            video_meta->format, video_meta->width,
            video_meta->height, GST_BUFFER_PTS(buffer));
  } while(0);

  return GST_PAD_PROBE_OK;
}

IPadProber::IPadProber(GstPad *pad, GstPadProbeType mask, IPadHandler handler)
  : pad_((GstPad*)gst_object_ref(pad))
  , probe_id_(0)
  , handler_(std::move(handler))
{
  do {
    ALOG_BREAK_IF(!pad_);

    probe_id_ = gst_pad_add_probe(pad_, mask, &on_pad_probe_callback, this, NULL);
    ALOG_BREAK_IF(probe_id_ == 0);

    ALOGD("IPadProber registered id:%u (from %s)", probe_id_, GST_PAD_NAME(pad_));
  } while(0);
}

IPadProber::~IPadProber()
{
  if (pad_) {
    if (probe_id_) {
      gst_pad_remove_probe(pad_, probe_id_);
      ALOGD("IPadProber unregistered id:%u", probe_id_);
    }
    gst_object_unref(pad_);
    pad_ = nullptr;
  }
}

GstPadProbeReturn IPadProber::on_pad_probe_callback(GstPad *pad,
        GstPadProbeInfo *info, gpointer data)
{
  IPadProber *const prober = static_cast<IPadProber*>(data);
  if (prober && prober->handler_)
    return prober->handler_(info);
  return GST_PAD_PROBE_OK;
}

}