#include "video_mixer.h"

#include "debug.h"

VideoMixer::VideoMixer()
  : bin_(nullptr)
  , mix_(nullptr)
  , caps_(nullptr)
  , tee_(nullptr)
{
}

VideoMixer::~VideoMixer()
{
}

gint VideoMixer::init()
{
  gint ret = -1;

  do {
    bin_ = gst_bin_new("video-mixer-bin");
    ALOG_BREAK_IF(!bin_);

    mix_ = gst_element_factory_make("glvideomixer", "video-mixer-gl");
    ALOG_BREAK_IF(!mix_);

    caps_ = gst_element_factory_make("capsfilter", "video-mixer-caps");
    ALOG_BREAK_IF(!caps_);

    tee_ = gst_element_factory_make("tee", "video-mixer-tee");
    ALOG_BREAK_IF(!tee_);

    g_object_set(G_OBJECT(mix_),
            "background", 1,
            NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, SCREEN_WIDTH,
            "height", G_TYPE_INT, SCREEN_HEIGHT,
            // "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(caps_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }
    ret = 0;
  } while(0);

  if (ret != 0) {
    deinit();
  }

  return ret;
}