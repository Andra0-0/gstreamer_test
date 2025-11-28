#ifndef MATRIX_VIDEO_INPUT_IMPL_URIDB_H
#define MATRIX_VIDEO_INPUT_IMPL_URIDB_H

#include <mutex>
#include <vector>
#include <unordered_map>

#include "media_input_intf.h"

namespace mmx {

using std::mutex;
using std::vector;
using std::unordered_map;

class MediaInputImplUridb : public MediaInputIntf {
public:
  MediaInputImplUridb();

  virtual ~MediaInputImplUridb();

  virtual gint init() override;

  virtual gint deinit() override;

  virtual gint start() override;

  virtual gint pause() override;

  virtual GstElement* get_bin() override;

  virtual GstPad* get_request_pad(bool is_video) override;

  virtual gint set_property(const string &name, const MetaMessagePtr &msg) override;

  // uridecodebin element signals callback
  static void on_uridb_pad_added(GstElement *obj, GstPad *pad, void *data);
  static void on_uridb_no_more_pads(GstElement *obj, void *data);
  static void on_uridb_element_added(GstElement *obj, GstElement *elem, void *data);
  static void on_uridb_unknown_type(GstElement *obj, GstPad *pad, GstCaps caps, void *data);

protected:
  GstPad* create_video_src_pad();
  void create_video_process(GstPad *pad);

  void create_audio_process(GstPad *pad);

private:
  mutex lock_;

  GstElement *bin_;
  GstElement *source_;

  bool video_linked_;
  gint video_pad_cnt_;
  GstElement *video_cvt_;
  GstElement *video_caps_;
  GstElement *video_tee_;
  // vector<GstPad*> video_pad_;
  unordered_map<string, GstElement*> video_queue_; // string padname, GstElement *queue

  GstElement *audio_tee_;

  // manager pad
  vector<GstElement*> pad_array_;
};

}

#endif // MATRIX_VIDEO_INPUT_IMPL_HDMI_H