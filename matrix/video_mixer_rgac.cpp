#include "video_mixer_rgac.h"

#include <sstream>

#include "debug.h"

namespace mmx {

VideoMixerRgac::VideoMixerRgac()
{
}

VideoMixerRgac::~VideoMixerRgac()
{
}

gint VideoMixerRgac::init()
{
  gint ret = -1;

  do {
    // TODO name
    bin_ = gst_bin_new(nullptr);
    ALOG_BREAK_IF(!bin_);

    mix_ = gst_element_factory_make("compositor", "rgacompositor");
    ALOG_BREAK_IF(!mix_);

    cvt_ = gst_element_factory_make("videoconvert", "video-mixer-cvt");
    ALOG_BREAK_IF(!cvt_);

    caps_ = gst_element_factory_make("capsfilter", "video-mixer-caps");
    ALOG_BREAK_IF(!caps_);

    tee_ = gst_element_factory_make("tee", "video-mixer-tee");
    ALOG_BREAK_IF(!tee_);

    queue_ = gst_element_factory_make("queue", "video-mixer-queue");
    ALOG_BREAK_IF(!queue_);

    g_object_set(G_OBJECT(mix_),
            "background", 1, // 1-black
            "ignore-inactive-pads", TRUE,
            "force-live", TRUE,
            "min-upstream-latency", 33333333,
            NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            "width", G_TYPE_INT, screen_width_,
            "height", G_TYPE_INT, screen_height_,
            "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(caps_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    gst_bin_add_many(GST_BIN(bin_), mix_, cvt_, caps_, tee_, queue_, NULL);
    if (!gst_element_link_many(mix_, cvt_, caps_, tee_, queue_, NULL)) {
      ALOGD("Failed to link elements");
    }
    if (!prober_out_) {
      GstPad *pad = gst_element_get_static_pad(mix_, "src");
      prober_out_ = std::make_shared<IPadProbeVideoInfo>("rgaout", pad);
      gst_object_unref(pad);
    }
    ret = 0;
  } while(0);

  if (ret != 0) {
    deinit();
  }

  return ret;
}

gint VideoMixerRgac::deinit()
{
  // TODO gst_element_factory_make failed?
}

// gint VideoMixerRgac::connect(VideomixConfig &cfg)
// {
//   gint ret = -1;
//   // GstElement *queue, *glupload, *glconvert;
//   GstPad *src_pad = nullptr, *sink_pad = nullptr;

//   do {
//     ALOG_BREAK_IF(srcpad_umap_.size() >= kVideomixMaxStream);
//     ALOG_BREAK_IF(cfg.src_pad_ == NULL);

//     if (srcpad_umap_.find(GST_PAD_NAME(cfg.src_pad_)) != srcpad_umap_.end()) {
//       ALOGD("VideoMixerRgac connect %s error, already exist connect", GST_PAD_NAME(cfg.src_pad_));
//       break;
//     }

//     src_pad = cfg.src_pad_;
//     gst_object_ref(src_pad);
//     sink_pad = gst_element_get_static_pad(cvt_, "sink");
//     ALOG_BREAK_IF(!sink_pad);

//     string ghost_pad_name = "video_sink_" + std::to_string(sinkpad_cnt_);
//     cfg.sink_pad_ = gst_ghost_pad_new(ghost_pad_name.c_str(), sink_pad);
//     gst_element_add_pad(bin_, cfg.sink_pad_);
//     gst_pad_set_active(cfg.sink_pad_, TRUE);
//     gst_object_unref(sink_pad);
//     sink_pad = cfg.sink_pad_;
//     sinkpad_cnt_++;

//     if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
//       ALOGD("VideoMixerRgac failed to link pad");
//       break;
//     }
//     gst_object_unref(src_pad);
//     src_pad = sink_pad = NULL;

//     srcpad_umap_.emplace(GST_PAD_NAME(cfg.src_pad_), cfg);
//     ret = 0;
//   } while(0);

//   if (ret != 0) {
//     if (src_pad) gst_object_unref(src_pad);
//     if (sink_pad) gst_object_unref(sink_pad);
//   }

//   return ret;
// }

gint VideoMixerRgac::connect(VideomixConfig &cfg)
{
  ALOG_TRACE;
  gint ret = -1;
  GstElement *queue;
  GstPad *src_pad = nullptr, *sink_pad = nullptr;

  do {
    ALOG_BREAK_IF(srcpad_umap_.size() >= kVideomixMaxStream);
    ALOG_BREAK_IF(cfg.src_pad_ == NULL);

    if (srcpad_umap_.find(GST_PAD_NAME(cfg.src_pad_)) != srcpad_umap_.end()) {
      ALOGD("VideoMixerRgac connect %s error, already exist connect", GST_PAD_NAME(cfg.src_pad_));
      break;
    }

    queue = gst_element_factory_make("queue", NULL);
    ALOG_BREAK_IF(!queue);

    gst_bin_add(GST_BIN(bin_), queue);

    src_pad = cfg.src_pad_;
    gst_object_ref(src_pad);
    sink_pad = gst_element_get_static_pad(queue, "sink");
    ALOG_BREAK_IF(!sink_pad);

    string ghost_pad_name = "video_sink_" + std::to_string(sinkpad_cnt_);
    cfg.sink_pad_ = gst_ghost_pad_new(ghost_pad_name.c_str(), sink_pad);
    gst_element_add_pad(bin_, cfg.sink_pad_);
    gst_pad_set_active(cfg.sink_pad_, TRUE);
    gst_object_unref(sink_pad);
    sink_pad = cfg.sink_pad_;
    sinkpad_cnt_++;

    // if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
    //   ALOGD("VideoMixerRgac failed to link pad");
    //   break;
    // }
    gst_object_unref(src_pad);
    src_pad = sink_pad = NULL;

    src_pad = gst_element_get_static_pad(queue, "src");
    ALOG_BREAK_IF(!src_pad);
    if (!prober_) {
      prober_ = std::make_shared<IPadProbeVideoInfo>("rgain", src_pad);
    }
    sink_pad = gst_element_get_request_pad(mix_, "sink_%u");
    ALOG_BREAK_IF(!sink_pad);
    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("VideoMixerRgac failed to link pad");
      break;
    }
    g_object_set(sink_pad,
            "xpos", cfg.style_.xpos,
            "ypos", cfg.style_.ypos,
            "width", cfg.style_.width,
            "height", cfg.style_.height,
            "sizing-policy", 1, // 不做拉伸 混合缩放策略
            NULL);
    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);

    srcpad_umap_.emplace(GST_PAD_NAME(cfg.src_pad_), cfg);
    gst_element_sync_state_with_parent(queue);
    ret = 0;
  } while(0);

  if (ret != 0) {
    if (src_pad) gst_object_unref(src_pad);
    if (sink_pad) gst_object_unref(sink_pad);
  }

  return ret;
}

gint VideoMixerRgac::disconnect(GstPad *src_pad)
{
  do {
    ALOG_BREAK_IF(srcpad_umap_.find(GST_PAD_NAME(src_pad)) == srcpad_umap_.end());
  } while(0);

  return 0;
}

gint VideoMixerRgac::disconnect_all()
{
  return 0;
}

string VideoMixerRgac::get_info()
{
  std::ostringstream oss;
  GstState state, pending;

  oss << "\n\t============ get_info ============\n"
      << "\tClass: VideoMixerRgac, bin name: " << GST_OBJECT_NAME(bin_) << "\n";

  if (bin_) {
    gst_element_get_state(bin_, &state, &pending, 0);
    oss << "\tBinState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }
  GstPad *sink_pad = gst_element_get_static_pad(bin_, "video_sink_0");
  if (sink_pad) {
    oss << "\tsink_pad active: " << gst_pad_is_active(sink_pad) << " linked: " << gst_pad_is_linked(sink_pad) << "\n";
    gst_object_unref(sink_pad);
  } else {
    oss << "\tError to get video_sink_0\n";
  }
  GstPad *src_pad = gst_pad_get_peer(sink_pad);
  if (src_pad) {
    oss << "\tGhost pad video_sink_0 linked! peer name: " << GST_PAD_NAME(src_pad) << "\n"
        << "\tsrc_pad active: " << gst_pad_is_active(src_pad) << " linked: " << gst_pad_is_linked(src_pad) << "\n";
    gst_object_unref(src_pad);
  } else {
    oss << "\tError to get peer pad\n";
  }

  // sink_pad = gst_element_get_static_pad(mix_, "sink_0");
  // if (sink_pad) {
  //   oss << "\tmix_sink_0 active: " << gst_pad_is_active(sink_pad) << " linked: " << gst_pad_is_linked(sink_pad) << "\n";
  //   gst_object_unref(sink_pad);
  // } else {
  //   oss << "\tError to get mix_sink_0\n";
  // }
  // src_pad = gst_pad_get_peer(sink_pad);
  // if (src_pad) {
  //   oss << "\tmix_sink_0 linked! peer name: " << GST_PAD_NAME(src_pad) << "\n"
  //       << "\tsrc_pad active: " << gst_pad_is_active(src_pad) << " linked: " << gst_pad_is_linked(src_pad) << "\n";
  //   gst_object_unref(src_pad);
  // } else {
  //   oss << "\tError to get peer pad\n";
  // }

  oss << "\t==================================";

  return oss.str();
}

GstPad* VideoMixerRgac::get_request_pad(const gchar *name)
{
  GstPad *pad = nullptr;

  do {
    if (g_str_has_prefix(name, "video_sink_")) {
      GstPad *sink_pad = gst_element_get_static_pad(cvt_, "sink");
      if (sink_pad) {
        pad = gst_ghost_pad_new("video_sink_0", sink_pad);
        gst_element_add_pad(bin_, pad);
        gst_pad_set_active(pad, TRUE);
        gst_object_unref(sink_pad);
      } else {
        ALOGD("VideoMixerRgac failed to get sink pad");
      }
    } else if (g_str_has_prefix(name, "video_src_")) {
      GstPad *queue_src_pad = gst_element_get_static_pad(queue_, "src");
      if (queue_src_pad) {
        pad = gst_ghost_pad_new("video_src_0", queue_src_pad);
        gst_element_add_pad(bin_, pad);
        gst_pad_set_active(pad, TRUE);
        gst_object_unref(queue_src_pad);
      } else {
        ALOGD("VideoMixerRgac failed to get queue src pad");
      }
    }
  } while(0);

  return pad;
}

}