#ifndef MATRIX_VIDEO_MIXER_H
#define MATRIX_VIDEO_MIXER_H

#include <gst/gst.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace mmx {

class VideoMixer;

using std::string;
using std::vector;
using std::shared_ptr;
using std::unordered_map;
using VideoMixerPtr = shared_ptr<VideoMixer>;

enum {
  kVideomixMaxStream = 8,
};

enum VideomixLayout {
  kLayoutInvalid = 0,
  kLayoutSingleScreen,
  kLayoutDualScreen,
  kLayoutThreeScreen,
};

struct VideomixStyle {
  gint xpos;
  gint ypos;
  gint width;
  gint height;
};

struct VideomixConfig {
  gint index_;
  GstPad *src_pad_;
  GstPad *sink_pad_;
  VideomixStyle style_;
  VideomixLayout layout_;
};

class VideoMixer {
public:
  VideoMixer();

  ~VideoMixer();

  gint init();

  gint deinit();

  gint connect(VideomixConfig &cfg);

  gint connect_many(vector<VideomixConfig> &cfg_arr);

  gint disconnect(GstPad *src_pad);

  gint disconnect_all();

  string get_info();

  GstElement* get_bin() { return bin_; }

  /**
   * @param name video_src_0 video_sink_%u
   */
  // GstPad* get_static_pad(const gchar *name);

  /**
   * @param name video_src_%u
   */
  GstPad* get_request_pad(const gchar *name);

  VideomixStyle get_style_from_layout(VideomixLayout layout, gint index);

private:
  GstElement *bin_;
  GstElement *mix_;
  /*Debug*/GstElement *cvt_;
  GstElement *caps_;
  GstElement *tee_;
  GstElement *queue_;

  gint screen_width_;
  gint screen_height_;
  VideomixLayout layout_;

  gint sinkpad_cnt_;
  unordered_map<string, VideomixConfig> srcpad_umap_;
};

} // namespace mmx

#endif // MATRIX_VIDEO_MIXER_H