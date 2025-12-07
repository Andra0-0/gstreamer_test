#ifndef MATRIX_VIDEO_INPUT_IMPL_HDMI_H
#define MATRIX_VIDEO_INPUT_IMPL_HDMI_H

#include <mutex>
#include <unordered_map>

#include "media_input_intf.h"

namespace mmx {

using std::mutex;
using std::unordered_map;

class MediaInputImplHdmi : public MediaInputIntf {
public:
  MediaInputImplHdmi();

  virtual ~MediaInputImplHdmi();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint start() override;

  virtual gint pause() override;

  virtual string get_info() override;

  virtual GstElement* get_bin() override;

  virtual GstPad* get_request_pad(bool is_video) override;

  virtual gint set_property(const string &name, const MetaMessagePtr &msg) override;

protected:
  GstPad* create_video_src_pad();

private:
  GstElement *bin_;
  GstElement *source_;
  GstElement *convert_;
  GstElement *capsfilter_;
  GstElement *input_selector_;
  GstElement *tee_;
  // GstElement *queue_;

  mutex lock_;
  // unordered_map<const char*, GstElement*> umap_queue_;
  gint video_pad_cnt_;
};

}

#endif // MATRIX_VIDEO_INPUT_IMPL_HDMI_H