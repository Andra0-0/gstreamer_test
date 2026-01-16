#include "media_input_module.h"

#include <sstream>

#include "media_input_impl_image.h"
#include "media_input_impl_hdmi.h"
#include "media_input_impl_uridb.h"
#include "media_matrix.h"
#include "media_define.h"
#include "debug.h"

namespace mmx {

mutex MediaInputModule::inslock_;
shared_ptr<MediaInputModule> MediaInputModule::instance_ = nullptr;

InputPadSwitch::InputPadSwitch(GstElement *bin, const IGhostPadManagerPtr &mana)
  : is_link_mainstream_(false)
  , is_curr_mainstream_(false)
  , pad_(nullptr)
  , selector_(nullptr)
  , queue_(nullptr)
{
  do {
    selector_ = gst_element_factory_make("input-selector", nullptr);
    ALOG_BREAK_IF(!selector_);
    // gst_object_ref(selector_);
    queue_ = gst_element_factory_make("queue", nullptr);
    ALOG_BREAK_IF(!queue_);

    g_object_set(G_OBJECT(selector_),
            "drop-backwards", TRUE,
            // "sync-mode", 1,
            // "sync-streams", FALSE,
            NULL);

    gst_bin_add_many(GST_BIN(bin), selector_, queue_, NULL);

    if (!gst_element_link_many(selector_, queue_, NULL)) {
      ALOGD("InputPadSwitch failed to link elements");
    }

    GstPad *queue_src_pad = gst_element_get_static_pad(queue_, "src");
    if (queue_src_pad) {
      pad_ = mana->add_pad(queue_src_pad);
      gst_object_unref(queue_src_pad);
    } else {
      ALOGD("Fatel error, get input-selector src pad failed");
    }
    gst_element_sync_state_with_parent(selector_);
    gst_element_sync_state_with_parent(queue_);
  } while(0);
}

InputPadSwitch::~InputPadSwitch()
{
  // if (selector_) {
  //   gst_object_unref(selector_);
  //   selector_ = nullptr;
  // }
  // if (pad_) {
  //   gst_object_unref(pad_);
  //   pad_ = nullptr;
  // }
}

/**
 * @brief 连接媒体流到input-selector，获取媒体流src pad失败也创建sink pad，等待后续重连
 * @param stream 输入媒体流
 * @param type 输入媒体流类型
 */
gint InputPadSwitch::connect(const MediaInputIntfPtr &stream, StreamType type)
{
  gint ret = -1;
  GstPadLinkReturn res;
  GstPad *src_pad, *sink_pad;
  string sink_pad_name;

  do {
    if (type == kTypeStreamMain) {
      ALOG_BREAK_IF(!stream_main_.empty());
      stream_main_ = stream->name();
    }

    sink_pad_name = "sink_" + std::to_string(type);
    sink_pad = gst_element_get_request_pad(selector_, sink_pad_name.c_str());
    ALOG_BREAK_IF(!sink_pad);

    src_pad = stream->get_request_pad({true, 1920, 1080, 30});
    if (!src_pad) {
      ALOGD("Failed to request src pad from %s", stream->name());
      break;
    }

    res = gst_pad_link(src_pad, sink_pad);
    if (res != GST_PAD_LINK_OK) {
      ALOGD("Failed to link src pad to sink pad, res: %d", res);
      break;
    }

    switch (type) {
      case kTypeStreamMain:
        is_link_mainstream_ = true;
        is_curr_mainstream_ = true;
        stream_curr_ = stream->name();
        g_object_set(selector_, "active-pad", sink_pad, NULL);
        break;
      case kTypeStreamFallback1:
        if (!is_link_mainstream_) {
          is_curr_mainstream_ = false;
          stream_curr_ = stream->name();
          g_object_set(selector_, "active-pad", sink_pad, NULL);
        }
        break;
      default:
        // Should not happen
        ALOGD("Fatel error, unknown stream type: %d", type);
        if (sink_pad) g_object_unref(sink_pad);
        return -1;
    }

    InputPadCase padcase;
    padcase.type_ = type;
    padcase.indev_src_ = stream;
    stream_umap_.emplace(stream->name(), std::move(padcase));

    ALOGD("Connect stream %s to %s success",
            stream->name(), GST_OBJECT_NAME(selector_));
    ret = 0;
  } while(0);

  if (sink_pad) {
    g_object_unref(sink_pad);
  }

  return ret;
}

/**
 * @brief 媒体流已准备完毕，尝试重连到input-selector
 * @param stream 输入媒体流
 * @param type 输入媒体流类型
 */
gint InputPadSwitch::reconnect(const MediaInputIntfPtr &stream, StreamType type)
{
  GstPad *src_pad = nullptr, *sink_pad = nullptr;
  GstPadLinkReturn res;

  do {
    ALOG_BREAK_IF(type != kTypeStreamMain);
    if (stream_main_ != stream->name()) break;

    sink_pad = gst_element_get_static_pad(selector_, "sink_0");
    if (!sink_pad) {
      ALOGD("Failed to get static src pad from input-selector");
      break;
    }

    src_pad = stream->get_request_pad({true, 1920, 1080, 30});
    if (!src_pad) {
      ALOGD("Failed to request src pad from %s", stream->name());
      break;
    }

    res = gst_pad_link(src_pad, sink_pad);
    if (res != GST_PAD_LINK_OK) {
      ALOGD("Failed to link src pad to sink pad, res: %d", res);
      break;
    }

    is_link_mainstream_ = true;
    is_curr_mainstream_ = true;
    stream_curr_ = stream->name();
    g_object_set(selector_, "active-pad", sink_pad, NULL);
    // TODO: 需要清空后续通道的视频帧缓存
    MediaMatrix::instance()->update();
    ALOGD("Reconnect stream %s to %s success",
            stream->name(), GST_OBJECT_NAME(selector_));
  } while(0);

  if (sink_pad) {
    g_object_unref(sink_pad);
  }

  return 0;
}

/**
 * @brief 查找该媒体流，有则断开连接，默认备用流不断开
 * @param stream 输入媒体流
 * @param type 输入媒体流类型
 */
gint InputPadSwitch::disconnect(const MediaInputIntfPtr &stream)
{
  unordered_map<string, InputPadCase>::iterator iterator;

  do {
    iterator = stream_umap_.find(stream->name());
    if (iterator == stream_umap_.end()) {
      break;
    }
    ALOG_BREAK_IF(iterator->second.type_ != kTypeStreamMain);

    unordered_map<string, InputPadCase>::iterator it;
    for (it = stream_umap_.begin(); it != stream_umap_.end(); ++it) {
      if (it->first != iterator->first) break;
    }
    ALOG_BREAK_IF(it == stream_umap_.end()); // no other stream
    if (stream_curr_ == stream->name()) {
      string pad_name = "sink_" + std::to_string(it->second.type_);
      GstPad *pad = gst_element_get_static_pad(selector_, pad_name.c_str());
      if (pad) {
        g_object_set(G_OBJECT(selector_), "active-pad", pad, NULL);
        gst_object_unref(pad);
      }
    }

    is_link_mainstream_ = false;
    is_curr_mainstream_ = false;
    stream_curr_ = it->second.indev_src_->name();
    stream_main_.clear();
    stream_umap_.erase(iterator);
  } while(0);

  return 0;
}

gint InputPadSwitch::sswitch(StreamType type)
{
  GstPad *sink_pad = nullptr;

  switch (type) {
    case kTypeStreamMain:
      if (!is_link_mainstream_ || is_curr_mainstream_) break;
      sink_pad = gst_element_get_static_pad(selector_, "sink_0");
      if (sink_pad) {
        g_object_set(G_OBJECT(selector_), "active-pad", sink_pad, NULL);
        gst_object_unref(sink_pad);
      }
      break;
    case kTypeStreamFallback1:
      sink_pad = gst_element_get_static_pad(selector_, "sink_1");
      if (sink_pad) {
        g_object_set(G_OBJECT(selector_), "active-pad", sink_pad, NULL);
        gst_object_unref(sink_pad);
      }
      break;
    default:
      break;
  }

  return 0;
}

MediaInputModule::MediaInputModule()
  : IMessageThread("MediaInModule")
  , indev_cnt_(0)
{
  ALOG_TRACE;
  do {
    indev_type_name_ = {
      {kMediaInputInvalid, "MediaInputInvalid"},
      {kMediaInputImage, "MediaInputImage"},
      {kMediaInputHdmi, "MediaInputHdmi"},
      {kMediaInputUridb, "MediaInputUridb"},
    };
    indev_array_.resize(kInputStreamMaxNum);

    bin_ = gst_bin_new("MediaInputModule");
    ALOG_BREAK_IF(!bin_);

    inpad_video_padmanage_ = std::make_shared<IGhostPadManager>(bin_, "video_src");

    indev_nosignal_ = create_indev_nosignal();
  } while(0);
}

MediaInputModule::~MediaInputModule()
{

}

MediaInputIntfPtr MediaInputModule::create(const MediaInputConfig &cfg)
{
  ALOG_TRACE;
  MediaInputIntfPtr ret = nullptr;

  do {
    ALOG_BREAK_IF(cfg.type_ == kMediaInputInvalid);
    ALOG_BREAK_IF(indev_cnt_ >= kInputStreamMaxNum);
    lock_core_.lock();

    int i = 0;
    for (; i < kInputStreamMaxNum; ++i) {
      if (indev_array_[i] == nullptr)
        break;
    }
    string name = indev_type_name_[cfg.type_] + std::to_string(i);
    switch (cfg.type_) {
      case kMediaInputImage:
        ret = std::make_shared<MediaInputImplImage>();
        break;
      case kMediaInputHdmi:
        ret = std::make_shared<MediaInputImplHdmi>();
        break;
      case kMediaInputUridb:
        ret = std::make_shared<MediaInputImplUridb>();
        break;
      default:
        ALOGD("Error video input type: %d", cfg.type_);
        break;
    }
    ALOG_BREAK_IF(ret == nullptr);
    indev_array_[i] = ret;
    indev_cnt_++;

    ret->id_ = i;
    ret->name_ = name;
    ret->uri_ = cfg.uri_;
    ret->src_name_ = cfg.srcname_;
    ret->width_ = cfg.width_;
    ret->height_ = cfg.height_;
    ret->signal_indev_video_pad_added.connect(this,
            &MediaInputModule::on_indev_video_pad_added);
    ret->signal_indev_no_more_pads.connect(this,
            &MediaInputModule::on_indev_no_more_pads);
    ret->signal_indev_end_of_stream.connect(this,
            &MediaInputModule::on_indev_end_of_stream);
    ALOG_BREAK_IF(0 != ret->init());

    GstElement *new_bin = ret->get_bin();
    ALOG_BREAK_IF(!new_bin);
    gst_bin_add(GST_BIN(bin_), new_bin);
    // gst_element_sync_state_with_parent(new_bin);

    // add_videoin_new(ret);
    ALOGD("MediaInputModule create %s success", ret->name_.c_str());
    lock_core_.unlock();
  } while(0);

  return ret;
}

void MediaInputModule::destroy(gint id)
{
  do {
    ALOG_BREAK_IF(id >= kInputStreamMaxNum || id < 0);
    lock_core_.lock();

    indev_array_[id]->deinit();
    indev_array_[id] = nullptr;
    indev_cnt_--;

    lock_core_.unlock();
  } while(0);
}

string MediaInputModule::get_info()
{
  std::ostringstream oss;

  for (auto it : indev_array_) {
    if (it == nullptr) continue;

    oss << it->get_info();
  }

  return oss.str();
}

void MediaInputModule::handle_message(const IMessagePtr &msg)
{
  int id;

  do {
    if (msg->what() == IMSG_INPUT_STREAM_IS_READY) {
      ALOG_BREAK_IF(!msg->find_int32("id", id));
      // gst_element_sync_state_with_parent(indev_array_[id]->get_bin());
      IMessagePtr imsg = std::make_shared<IMessage>(
              IMSG_PIPE_RESET_PLAYING, MediaMatrix::instance());
      imsg->send();
    } else if (msg->what() == IMSG_INPUT_DEVICE_RETRY) {
      ALOG_BREAK_IF(!msg->find_int32("id", id));
      ALOGD("Attempting soft-restart of input bin %s", indev_array_[id]->name());
      indev_array_[id]->start();
    }
  } while(0);
}

/**
 * MediaInputModule内部创建，无信号图片输入源，作为备用流使用
 */
MediaInputIntfPtr MediaInputModule::create_indev_nosignal()
{
  // ALOG_TRACE;
  MediaInputIntfPtr result = nullptr;

  do {
    ALOG_BREAK_IF(indev_cnt_ >= kInputStreamMaxNum);

    int i = 0;
    for (; i < kInputStreamMaxNum; ++i) {
      if (indev_array_[i] == nullptr)
        break;
    }
    string name = indev_type_name_[kMediaInputImage] + std::to_string(i);
    result = std::make_shared<MediaInputImplImage>();
    ALOG_BREAK_IF(result == nullptr);
    indev_array_[i] = result;
    indev_cnt_++;

    result->id_ = i;
    result->name_ = name;
    result->uri_ = PATH_IMAGE_NOSIGNAL;
    result->src_name_ = "InputErrorSignal";
    ALOG_BREAK_IF(0 != result->init());

    GstElement *new_bin = result->get_bin();
    ALOG_BREAK_IF(!new_bin);
    gst_bin_add(GST_BIN(bin_), new_bin);

    ALOGD("MediaInputModule create %s success", result->name_.c_str());
  } while(0);

  return result;
}

/**
 * 外部调用，MediaInputModule会尝试创建，如果输入媒体流未准备好
 * 会切换到备用流，直到媒体流准备好后切换回主流
 */
GstPad* MediaInputModule::get_request_pad(const MediaInputIntfPtr &ptr, bool is_video)
{
  GstPad *new_pad = nullptr;

  do {
    if (is_video) {
      new_pad = create_video_src_pad(ptr);
    } else {
      // TODO
    }
  } while(0);

  return new_pad;
}

/**
 * 视频流准备完成
 * 检查所有input-selector，是否需要切换备用流到主流
 */
void MediaInputModule::on_indev_video_pad_added(const MediaInputIntf *ptr)
{
  ALOG_TRACE;
  MediaInputIntfPtr input;

  do {
    ALOG_BREAK_IF(!ptr);
    input = indev_array_[ptr->id()];

    for (auto it : inpad_video_switch_) {
      if (it.second->main_stream_name() == input->name()) {
        it.second->reconnect(input);
      }
    }
    IMessagePtr imsg = std::make_shared<IMessage>(IMSG_INPUT_STREAM_IS_READY, this);
    imsg->set_int32("id", ptr->id());
    imsg->send();
  } while(0);
}

void MediaInputModule::on_indev_no_more_pads(const MediaInputIntf *ptr)
{
  ALOG_TRACE;
  MediaInputIntfPtr input;

  do {
    ALOG_BREAK_IF(!ptr);
    // input = indev_array_[ptr->id()];

    // gst_element_sync_state_with_parent(input->get_bin());
    // /*Debug*/ALOGD("\e[1;31m%s no more pads\e[0m", input->name());
  } while(0);
}

void MediaInputModule::on_indev_end_of_stream(const MediaInputIntf *ptr)
{
  ALOG_TRACE;
  MediaInputIntfPtr input;

  do {
    ALOG_BREAK_IF(!ptr);
    input = indev_array_[ptr->id()];

    for (auto it : inpad_video_switch_) {
      if (it.second->main_stream_name() == input->name()) {
        it.second->sswitch(InputPadSwitch::kTypeStreamFallback1);
      }
    }
  } while(0);
}

/**
 * MediaMatrix先检查GstMessage，错误发生在MediaInputModule内部，则从这处理
 */
void MediaInputModule::handle_bus_msg_error(GstBus *bus, GstMessage *msg)
{
  ALOG_TRACE;
  // GstElement *elem;
  // GstElementFactory *factory;

  do {
    for (auto input : indev_array_) {
      if (!input) continue;

      GstElement *bin = input->get_bin();
      if (!bin) {
        ALOGD("Error to get bin from %s", input->name());
        continue;
      }
      if (gst_object_has_ancestor(msg->src, GST_OBJECT(bin))) {
        ALOGD("Detected error in input %s (id=%d)", input->name(), input->id());

        for (auto it : inpad_video_switch_) {
          if (it.second->main_stream_name() == input->name()) {
            it.second->sswitch(InputPadSwitch::kTypeStreamFallback1);
          }
        }
        input->handle_bus_msg_error(bus, msg);

        IMessagePtr imsg = std::make_shared<IMessage>(IMSG_INPUT_DEVICE_RETRY, this);
        imsg->set_int32("id", input->id());
        imsg->send(5000000); // 5s
        break;
      }
    }
  } while(0);
}

/**
 * 在MediaInputModule Bin上申请一个src pad
 * 尝试连接输入源视频流，若失败则切换到备用流
 */
GstPad* MediaInputModule::create_video_src_pad(const MediaInputIntfPtr &ptr)
{
  ALOG_TRACE;
  InputPadSwitchPtr inpad_switch;
  GstPad *pad = nullptr;

  do {
    lock_core_.lock();
    for (auto it : inpad_video_switch_) {
      if (it.second->main_stream_name() == ptr->name()) {
        pad = it.second->get_pad();
        lock_core_.unlock();
        return pad;
      }
    }

    inpad_switch = std::make_shared<InputPadSwitch>(bin_, inpad_video_padmanage_);
    pad = inpad_switch->get_pad();

    inpad_switch->connect(ptr, InputPadSwitch::kTypeStreamMain);
    inpad_switch->connect(indev_nosignal_, InputPadSwitch::kTypeStreamFallback1);

    inpad_video_switch_.emplace(GST_PAD_NAME(pad), std::move(inpad_switch));
    lock_core_.unlock();
  } while(0);

  return pad;
}

} // namespace mmx