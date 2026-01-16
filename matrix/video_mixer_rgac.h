#ifndef MATRIX_VIDEO_MIXER_RGAC_H
#define MATRIX_VIDEO_MIXER_RGAC_H

#include "video_mixer.h"

namespace mmx {

class VideoMixerRgac : public VideoMixer {
public:
  VideoMixerRgac();

  virtual ~VideoMixerRgac();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint connect(VideomixConfig &cfg) override;

  virtual gint disconnect(GstPad *src_pad) override;

  virtual gint disconnect_all() override;

  virtual string get_info() override;

  /**
   * @param name video_src_0 video_sink_%u
   */
  // GstPad* get_static_pad(const gchar *name);

  /**
   * @param name video_src_%u
   */
  virtual GstPad* get_request_pad(const gchar *name) override;

private:

};

} // namespace mmx

#endif // MATRIX_VIDEO_MIXER_RGAC_H