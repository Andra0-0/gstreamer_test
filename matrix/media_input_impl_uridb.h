#ifndef MATRIX_VIDEO_INPUT_IMPL_URIDB_H
#define MATRIX_VIDEO_INPUT_IMPL_URIDB_H

#include <mutex>
#include <vector>
#include <unordered_map>

#include "media_input_intf.h"
#include "ipad_prober.h"
#include "ighost_pad_manager.h"

namespace mmx {

using std::mutex;
using std::vector;
using std::unordered_map;
using std::recursive_mutex;

class MediaInputImplUridb : public MediaInputIntf {
public:
  MediaInputImplUridb();

  virtual ~MediaInputImplUridb();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint start() override;

  virtual gint pause() override;

  virtual string get_info() override;

  virtual GstPad* get_request_pad(const ReqPadInfo &info) override;

  virtual gint set_property(const IMessagePtr &msg) override;

  virtual void handle_bus_msg_error(GstBus *bus, GstMessage *msg) override;

  // uridecodebin element signals callback
  static void on_uridb_pad_added(GstElement *obj, GstPad *pad, void *data);
  static void on_uridb_no_more_pads(GstElement *obj, void *data);
  static void on_uridb_element_added(GstElement *obj, GstElement *elem, void *data);
  static void on_uridb_unknown_type(GstElement *obj, GstPad *pad, GstCaps caps, void *data);

protected:
  GstPad* inter_get_video_pad();

  void inter_handle_video_pad(GstPad *pad);

  void inter_handle_audio_pad(GstPad *pad);

  GstPadProbeReturn on_handle_end_of_stream(GstPadProbeInfo *info);

private:
  recursive_mutex lock_;

  // GstElement *bin_;
  GstElement *source_;
  GstElement *video_cvt_;
  GstElement *video_scale_;
  GstElement *video_caps_;
  GstElement *video_tee_;

  // vector<GstPad*> video_pad_;
  unordered_map<string, GstElement*> video_queue_; // string padname, GstElement *queue

  GstElement *audio_tee_;

  IPadProberPtr prober_;

  // GstElement *fakesink_;
  // GstElement *fakequeue_;

  // manager pad
  // gint video_pad_cnt_;
  // vector<GstElement*> pad_array_;
  // IGhostPadManagerPtr pad_manager_;

  bool video_linked_; // 视频流是否已经链接
};

}

#endif // MATRIX_VIDEO_INPUT_IMPL_HDMI_H