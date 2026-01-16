#ifndef MATRIX_VIDEO_INPUT_INTF_H
#define MATRIX_VIDEO_INPUT_INTF_H

#include <gst/gst.h>
#include <string>
#include <memory>
#include <unordered_map>

#include "noncopyable.h"
#include "imessage.h"
#include "ighost_pad_manager.h"
#include "3rdparty/sigslot/sigslot.h"

namespace mmx {

class MediaInputIntf;

using std::string;
using std::shared_ptr;
using std::unordered_map;
using MediaInputIntfPtr = shared_ptr<MediaInputIntf>;

struct ReqPadInfo {
  bool is_video_;
  gint width_;
  gint height_;
  gint framerate_;
};

class MediaInputIntf : public NonCopyable {
public:
  enum MediaStreamState {
    kStreamStateInvalid,
    kStreamStateReady,
    kStreamStatePlaying,
    kStreamStatePause,
  };

  MediaInputIntf();

  virtual ~MediaInputIntf() = default;

  /**
   * 媒体流初始化或开始解析
   */
  virtual gint init() = 0;

  /**
   * 媒体流销毁
   */
  virtual gint deinit() = 0;

  /**
   * 媒体流播放（在流解析完成之后）
   */
  virtual gint start() = 0;

  /**
   * 媒体流暂停
   */
  virtual gint pause() = 0;

  /**
   * 获取当前媒体流信息
   */
  virtual string get_info() = 0;

  /**
   * 获取Bin加入父级
   */
  virtual GstElement* get_bin() { return bin_; }

  // TODO
  // virtual GstPad* get_static_pad(const gchar *name) = 0;

  /**
   * 尝试获取音视频src pad，若媒体流未准备好返回nullptr
   */
  virtual GstPad* get_request_pad(const ReqPadInfo &info) = 0;

  virtual gint set_property(const IMessagePtr &msg) { return 0; }

  virtual gint id() const { return id_; }

  virtual gint state() const { return state_; }

  virtual const char *name() const { return name_.c_str(); }

  virtual const char *src_name() const { return src_name_.c_str(); }

  virtual void handle_bus_msg_error(GstBus *bus, GstMessage *msg) = 0;

  sigslot::signal1<const MediaInputIntf*> signal_indev_video_pad_added;
  sigslot::signal1<const MediaInputIntf*> signal_indev_no_more_pads;
  sigslot::signal1<const MediaInputIntf*> signal_indev_end_of_stream;

  friend class MediaInputModule;
protected:
  gint    id_; // 标识id，MediaInputModule生成
  gint    state_; // 输入源状态
  string  uri_; // 资源地址
  string  name_; // 标识名，MediaInputModule生成
  string  src_name_; // 用户命名输入源
  gint    width_; // 视频宽度
  gint    height_; // 视频高度

  GstElement *bin_; // 输入源封装

  unordered_map<string, ReqPadInfo> pad_info_;
  IGhostPadManagerPtr apad_mngr_; // 输入媒体流音频连接管理
  IGhostPadManagerPtr vpad_mngr_; // 输入媒体流视频连接管理
};

} // namespace mmx

#endif // MATRIX_VIDEO_INPUT_INTF_H