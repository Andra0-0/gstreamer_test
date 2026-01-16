#include "ipad_prober.h"

#include <gst/video/video.h> // GstVideoMeta
#include <functional> // std::bind

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

IPadProbeVideoInfo::IPadProbeVideoInfo(string name, GstPad *pad)
  : prober_(nullptr)
  , name_(std::move(name))
  , mask_(GST_PAD_PROBE_TYPE_BUFFER)
  , first_frame_(true)
  , frame_cnt_(0)
{
  IPadProber::IPadHandler handler;
  handler = std::bind(&mmx::IPadProbeVideoInfo::report_video_rate,
          this, std::placeholders::_1);
  prober_ = std::make_shared<IPadProber>(pad,
          GST_PAD_PROBE_TYPE_BUFFER, handler);
}

IPadProbeVideoInfo::~IPadProbeVideoInfo()
{
  prober_ = nullptr;
}

GstPadProbeReturn IPadProbeVideoInfo::report_video_rate(GstPadProbeInfo *info)
{
  GstBuffer *buffer;
  GstVideoMeta *video_meta;

  do {
    if (!(info->type & mask_)) return GST_PAD_PROBE_OK;

    buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    ALOG_BREAK_IF(!buffer);

    video_meta = gst_buffer_get_video_meta(buffer);
    ALOG_BREAK_IF(!video_meta);

    frame_cnt_++;

    if (first_frame_) {
      GstPad *pad = prober_->get_pad();
      if (pad) {
        GstCaps *caps = gst_pad_get_current_caps(pad);
        if (!caps) {
          // 当前Caps为空，尝试获取协商后的Caps
          caps = gst_pad_get_allowed_caps(pad);
        }
        if (caps && gst_caps_get_size(caps) > 0) {
          const GstStructure *s = gst_caps_get_structure(caps, 0);
          const gchar *media_type = gst_structure_get_name(s);
          if (g_str_has_prefix(media_type, "video/")) {
            gint fps_n = 0, fps_d = 0;
            gdouble fps = 0.0;

            if (gst_structure_get_fraction(s, "framerate", &fps_n, &fps_d)) {
              if (fps_d > 0) {
                theoretical_fps_ = static_cast<double>(fps_n) / fps_d;
                ALOGD("theoretical video fps: %.2f(%d/%d)",
                        theoretical_fps_, fps_n, fps_d);
              }
            } else if (gst_structure_get_double(s, "framerate", &fps)) {
              theoretical_fps_ = fps;
              ALOGD("theoretical video fps: %.2f", theoretical_fps_);
            }
          }
        } else {
          ALOGD("Failed to get current/allowed caps from pad");
        }
      }
      gettimeofday(&timestamp_, NULL);
      first_frame_ = false;
      break;
    } else {
      timeval now;
      gettimeofday(&now, NULL);
      size_t time_diff_us = (now.tv_sec - timestamp_.tv_sec) * 1000000UL +
              (now.tv_usec - timestamp_.tv_usec);
      if (time_diff_us >= 1000000UL) {
        double fps = (double)frame_cnt_ / (time_diff_us / 1000000.0);
        ALOGD("%s fps: %.2f fps_t:%.2f", name_.c_str(), fps, theoretical_fps_);
        timestamp_ = now;
        frame_cnt_ = 0;
      }
    }
  } while(0);

  return GST_PAD_PROBE_OK;
}

IPadProbeVideoInfo& IPadProbeVideoInfo::operator=(const IPadProbeVideoInfo &other)
{
  if (this != &other) {
    prober_ = std::move(other.prober_);
    theoretical_fps_ = other.theoretical_fps_;
    timestamp_ = other.timestamp_;
    first_frame_ = other.first_frame_;
    frame_cnt_ = other.frame_cnt_;
  }
  return *this;
}

}