#ifndef MATRIX_VIDEO_INPUT_MODULE_H
#define MATRIX_VIDEO_INPUT_MODULE_H

#include <list>
#include <memory>
#include <mutex>
#include <vector>
#include <gst/gst.h>

#include "noncopyable.h"
#include "media_input_intf.h"

namespace mmx {

class MediaInputModule;

using std::list;
using std::mutex;
using std::vector;
using std::recursive_mutex;
using MediaInputModulePtr = shared_ptr<MediaInputModule>;

enum VideoInputType {
  kVideoInputInvalid,
  kVideoInputImage,
  kVideoInputHdmi,
  kVideoInputUridb,
};

/**
 * @param inpad_ MediaInputModule bin src pad
 * @param inselect_ MediaInputModule bin input-selector
 * @param indev_main_ 主输入源
 * @param indev_curr_ 当前输入源
 * @param indev_list_ 输入源链表
 */
struct VideoRequestPad {
  GstPad *inpad_;
  GstElement *inselect_;
  string indev_main_;
  string indev_curr_;
  bool indev_main_linked_;
  list<MediaInputIntfPtr> indev_list_;
};

/**
 * 视频输入流管理模块，负责创建销毁视频流
 */
class MediaInputModule : public NonCopyable {
public:
  enum {
    kVideoInputMaxNum = 16,
  };

  static inline MediaInputModulePtr instance() {
    if (!instance_) {
      std::lock_guard<mutex> _l(lock_ins_);
      if (instance_) return instance_;

      instance_ = std::make_shared<MediaInputModule>();
    }
    return instance_;
  }

  MediaInputModule();

  virtual ~MediaInputModule();

  MediaInputIntfPtr create(VideoInputType type);

  void destroy(gint id);

  GstElement *get_bin() { return videoin_bin_; }

  // GstPad* get_src_pad(const char *padname);

  /**
   * 外部调用，MediaInputModule会尝试创建，如果输入媒体流未准备好
   * 会切换到备用流，直到媒体流准备好后切换回主流
   */
  GstPad* get_request_pad(const MediaInputIntfPtr &ptr, bool is_video=true);

  /**
   * 媒体流准备完成
   * 尝试检查所有src pad，是否需要切换备用流到主流
   */
  void on_videoin_is_ready(MediaInputIntf *ptr);

  /**
   * MediaMatrix先检查GstMessage，错误发生在MediaInputModule内部，则从这处理
   */
  void on_handle_bus_msg_error(GstBus *bus, GstMessage *msg);

protected:
  MediaInputIntfPtr create_videoin_err();

  /**
   * 在MediaInputModule Bin上申请一个src pad
   * 尝试连接输入源视频流，若失败则切换到备用流
   */
  GstPad* create_video_src_pad(const MediaInputIntfPtr &ptr);

  void connect_selector(const MediaInputIntfPtr &ptr);
  void switch_selector(const MediaInputIntfPtr &ptr, bool open);

private:
  static MediaInputModulePtr instance_;
  static mutex lock_ins_;

  recursive_mutex lock_core_;
  gint videoin_cnt_;
  MediaInputIntfPtr videoin_err_;
  vector<MediaInputIntfPtr> videoin_array_;

  /**
   * @param string pad名称：video_src_%u
   * @param VideoRequestPad 输入源链表及输入选择
   */
  unordered_map<string, VideoRequestPad> reqpad_umap_;
  gint reqpad_cnt_;

  GstElement *videoin_bin_;
  // vector<GstElement*> videoinput_selector_;
};

} // namespace mmx

#endif // MATRIX_VIDEO_INPUT_MODULE_H