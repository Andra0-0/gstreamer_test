#ifndef MATRIX_VIDEO_INPUT_IMPL_IMAGE_H
#define MATRIX_VIDEO_INPUT_IMPL_IMAGE_H

#include <mutex>
#include <unordered_map>

#include "media_input_intf.h"
#include "ipad_prober.h"
#include "ighost_pad_manager.h"

namespace mmx {

using std::mutex;
using std::unordered_map;

class MediaInputImplImage : public MediaInputIntf {
public:
  MediaInputImplImage();

  virtual ~MediaInputImplImage();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint start() override;

  virtual gint pause() override;

  virtual string get_info() override;

  virtual GstElement* get_bin() override;

  virtual GstPad* get_request_pad(bool is_video) override;

  virtual gint set_property(const IMessagePtr &msg) override;

protected:
  GstPad* create_video_src_pad();

private:
  GstElement *bin_;
  GstElement *source_;
  GstElement *parser_;
  GstElement *decoder_;
  GstElement *convert_;
  GstElement *capsfilter_;
  GstElement *imagefreeze_;
  GstElement *videorate_;
  GstElement *tee_;

  mutex lock_;
  // gint video_pad_cnt_;
  // unordered_map<const char*, GstElement*> umap_queue_;
  IPadProberPtr prober_;
  IGhostPadManagerPtr pad_manager_;
};

}

#endif // MATRIX_VIDEO_INPUT_IMPL_IMAGE_H