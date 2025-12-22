#ifndef MATRIX_VIDEO_INPUT_MODULE_H
#define MATRIX_VIDEO_INPUT_MODULE_H

#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include <gst/gst.h>

#include "media_input_intf.h"
#include "ipad_prober.h"
#include "imessage_thread.h"
#include "ighost_pad_manager.h"

namespace mmx {

class InputPadSwitch;
class MediaInputModule;

using std::list;
using std::mutex;
using std::vector;
using std::recursive_mutex;
using InputPadSwitchPtr = shared_ptr<InputPadSwitch>;
using MediaInputModulePtr = MediaInputModule*;

enum MediaInputType {
  kMediaInputInvalid,
  kMediaInputImage,
  kMediaInputHdmi,
  kMediaInputUridb,
};

enum {
  kInputStreamMaxNum = 16,
};

struct MediaInputConfig {
  MediaInputType  type_;
  string          uri_;
  string          srcname_;
  gint            width_;
  gint            height_;

  MediaInputConfig() : type_(kMediaInputInvalid), width_(1920), height_(1080) {}
};

class InputPadSwitch : public NonCopyable {
public:
  enum StreamType {
    kTypeStreamMain = 0, // input-selector sink_0
    kTypeStreamFallback1 = 1, // input-selector sink_1
  };
  struct InputPadCase {
    // unordered_map<string, GstPad*> a;
    StreamType type_;
    // GstPad *indev_src_pad_;
    MediaInputIntfPtr indev_src_;
  };

  InputPadSwitch(GstElement *bin, const IGhostPadManagerPtr &mana);
  ~InputPadSwitch();

  GstPad* get_pad() { return pad_; }

  gint connect(const MediaInputIntfPtr &stream, StreamType type);
  gint reconnect(const MediaInputIntfPtr &stream, StreamType type=kTypeStreamMain);
  gint disconnect(const MediaInputIntfPtr &stream);

  gint sswitch(StreamType type);

  bool linked_mainstream() { return is_link_mainstream_; }
  bool current_mainstream() { return is_curr_mainstream_; }

  string curr_stream_name() { return stream_curr_; }
  string main_stream_name() { return stream_main_; }

private:
  bool is_link_mainstream_; // Main stream might not ready
  bool is_curr_mainstream_; // Current stream is mainstream or not

  GstPad *pad_; // MediaInputModule bin ghost-pad
  GstElement *selector_; // MediaInputModule bin input-selector
  GstElement *queue_; // MediaInputModule bin queue

  string stream_main_; // Name of main stream
  string stream_curr_; // Name of current stream

  // string: stream name, InputPadCase: stream info
  unordered_map<string, InputPadCase> stream_umap_;
};

/**
 * 视频输入流管理模块，负责创建销毁视频流
 */
class MediaInputModule : public IMessageThread, public sigslot::has_slots<> {
public:
  static inline MediaInputModulePtr instance() {
    if (!instance_) {
      std::lock_guard<mutex> _l(inslock_);
      if (instance_) return instance_.get();

      instance_ = shared_ptr<MediaInputModule>(new MediaInputModule());
    }
    return instance_.get();
  }

  virtual ~MediaInputModule();

  MediaInputIntfPtr create(const MediaInputConfig &cfg);

  void destroy(gint id);

  string get_info();

  GstElement *get_bin() { return bin_; }

  // GstPad* get_src_pad(const char *padname);

  GstPad* get_request_pad(const MediaInputIntfPtr &ptr, bool is_video=true);

  void handle_bus_msg_error(GstBus *bus, GstMessage *msg);

  void on_indev_video_pad_added(const MediaInputIntf *ptr);
  void on_indev_no_more_pads(const MediaInputIntf *ptr);
  void on_indev_end_of_stream(const MediaInputIntf *ptr);

protected:
  MediaInputModule();

  virtual void handle_message(const IMessagePtr &msg) override;

  MediaInputIntfPtr create_indev_nosignal();

  GstPad* create_video_src_pad(const MediaInputIntfPtr &ptr);

private:
  static mutex                        inslock_;
  static shared_ptr<MediaInputModule> instance_;

  GstElement *bin_;

  // 输入媒体流
  gint                            indev_cnt_;
  MediaInputIntfPtr               indev_nosignal_;
  vector<MediaInputIntfPtr>       indev_array_;
  unordered_map<int,const char*>  indev_type_name_;

  // 输入媒体流连接管理
  unordered_map<string,
                InputPadSwitchPtr>  inpad_video_switch_;
  IGhostPadManagerPtr               inpad_video_padmanage_;

  recursive_mutex lock_core_;
};

} // namespace mmx

#endif // MATRIX_VIDEO_INPUT_MODULE_H