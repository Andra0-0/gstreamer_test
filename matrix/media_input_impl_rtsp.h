#ifndef MATRIX_VIDEO_INPUT_IMPL_RTSP_H
#define MATRIX_VIDEO_INPUT_IMPL_RTSP_H

#include <mutex>
#include <unordered_map>

#include "media_input_intf.h"
#include "ighost_pad_manager.h"
#include "ipad_prober.h"

namespace mmx {

using std::mutex;
using std::unordered_map;

class MediaInputImplRtsp : public MediaInputIntf {
public:
  MediaInputImplRtsp();

  virtual ~MediaInputImplRtsp();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint start() override;

  virtual gint pause() override;

  virtual string get_info() override;

  virtual GstPad* get_request_pad(const ReqPadInfo &info) override;

  virtual gint set_property(const IMessagePtr &msg) override;

  virtual void handle_bus_msg_error(GstBus *bus, GstMessage *msg) override;

protected:
  GstPad* create_video_src_pad();

private:
  // GstElement *bin_;
  GstElement *source_;
  GstElement *convert_;
  GstElement *srccaps_;
  GstElement *tee_;

  mutex lock_;
  /*Debug*/IPadProbeVideoInfoPtr prober_;
};

}

#endif // MATRIX_VIDEO_INPUT_IMPL_RTSP_H