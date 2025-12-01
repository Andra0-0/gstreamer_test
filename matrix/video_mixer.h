#ifndef VIDEO_MIXER_H
#define VIDEO_MIXER_H

#include <gst/gst.h>

#include "input_intf.h"
#include "output_intf.h"

#define MIX_STREAM_MAX 4
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

enum videomix_style {
  VIDEOMIX_LAYOUT_PIP = 0,
  VIDEOMIX_LAYOUT_SPLIT,
  VIDEOMIX_LAYOUT_TRIPLE,
};

struct videomix_layout_t {
  gint xpos;
  gint ypos;
  gint width;
  gint height;
};

struct videomix_config_t {
  gchar *name;
  gint stream_index;
  // videomix_layout_t layout;
};

class VideoMixer {
public:
  VideoMixer();

  ~VideoMixer();

  gint init();

  gint deinit();

  // gint connect(InputIntfPtr input, videomix_layout_t layout);

  gint update();

  gint disconnect_all();

  GstElement* get_bin() { return bin_; }

  /**
   * @param name video_src
   */
  GstPad* get_static_pad(const gchar *name);

  /**
   * @param name video_sink_%u
   */
  GstPad* get_request_pad(const gchar *name);

protected:
  gint setVideoMixLayout(gint input_count);
  gint setVideoMixLayout(videomix_style style);

private:
  GstElement *bin_;
  GstElement *mix_;
  GstElement *caps_;
  GstElement *tee_;
  GstElement *queue_;

  videomix_layout_t layouts_[MIX_STREAM_MAX];
};

#endif // VIDEO_MIXER_H