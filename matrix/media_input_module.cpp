#include "media_input_module.h"

#include "media_input_impl_image.h"
#include "media_input_impl_hdmi.h"
#include "media_input_impl_uridb.h"
#include "media_matrix.h"
#include "debug.h"

namespace mmx {

MediaInputModulePtr MediaInputModule::instance_ = nullptr;
mutex MediaInputModule::lock_ins_;

const char *videoin_name[] = {
  "VideoInputInvalid",
  "VideoInputImage",
  "VideoInputHdmi",
  "VideoInputUridb"
};

InputPadSwitch::InputPadSwitch(GstElement *bin, const gchar *name)
  : is_link_mainstream_(false)
  , is_curr_mainstream_(false)
  , pad_(nullptr)
  , selector_(nullptr)
{
  do {
    selector_ = gst_element_factory_make("input-selector", nullptr);
    ALOG_BREAK_IF(!selector_);
    // gst_object_ref(selector_);
    g_object_set(G_OBJECT(selector_),
            "drop-backwards", TRUE,
            // "sync-mode", 1,
            // "sync-streams", FALSE,
            NULL);

    gst_bin_add(GST_BIN(bin), selector_);

    GstPad *selector_src_pad = gst_element_get_static_pad(selector_, "src");
    if (selector_src_pad) {
      pad_ = gst_ghost_pad_new(name, selector_src_pad);
      gst_element_add_pad(bin, pad_);
      gst_object_unref(selector_src_pad);
    } else {
      ALOGD("Fatel error, get input-selector src pad failed");
    }
    gst_element_sync_state_with_parent(selector_);
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
    sink_pad = gst_element_request_pad_simple(selector_, sink_pad_name.c_str());
    ALOG_BREAK_IF(!sink_pad);

    src_pad = stream->get_request_pad(true);
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
  GstPad *src_pad, *sink_pad;
  GstPadLinkReturn res;

  do {
    ALOG_BREAK_IF(type != kTypeStreamMain);

    sink_pad = gst_element_get_static_pad(selector_, "sink_0");
    if (!sink_pad) {
      ALOGD("Failed to get static src pad from input-selector");
      break;
    }

    src_pad = stream->get_request_pad(true);
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
  : videoin_cnt_(0)
{
  ALOG_TRACE;
  do {
    videoin_array_.resize(kVideoInputMaxNum);

    videoin_bin_ = gst_bin_new("MediaInputModule");
    ALOG_BREAK_IF(!videoin_bin_);

    videoin_err_ = create_videoin_err();
  } while(0);
}

MediaInputModule::~MediaInputModule()
{

}

MediaInputIntfPtr MediaInputModule::create(VideoInputType type)
{
  ALOG_TRACE;
  MediaInputIntfPtr ret = nullptr;

  do {
    ALOG_BREAK_IF(type == kVideoInputInvalid);
    ALOG_BREAK_IF(videoin_cnt_ >= kVideoInputMaxNum);
    lock_core_.lock();

    int i = 0;
    for (; i < kVideoInputMaxNum; ++i) {
      if (videoin_array_[i] == nullptr)
        break;
    }
    string name = videoin_name[type] + std::to_string(i);
    switch (type) {
      case kVideoInputImage:
        ret = std::make_shared<MediaInputImplImage>();
        break;
      case kVideoInputHdmi:
        ret = std::make_shared<MediaInputImplHdmi>();
        break;
      case kVideoInputUridb:
        ret = std::make_shared<MediaInputImplUridb>();
        break;
      default:
        ALOGD("Error video input type: %d", type);
        break;
    }
    ALOG_BREAK_IF(ret == nullptr);
    videoin_array_[i] = ret;
    videoin_cnt_++;

    ret->id_ = i;
    ret->name_ = name;
    ALOG_BREAK_IF(0 != ret->init());

    GstElement *new_bin = ret->get_bin();
    ALOG_BREAK_IF(!new_bin);
    gst_bin_add(GST_BIN(videoin_bin_), new_bin);
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
    ALOG_BREAK_IF(id >= kVideoInputMaxNum || id < 0);
    lock_core_.lock();

    videoin_array_[id]->deinit();
    videoin_array_[id] = nullptr;
    videoin_cnt_--;

    lock_core_.unlock();
  } while(0);
}

string MediaInputModule::get_info()
{

}

MediaInputIntfPtr MediaInputModule::create_videoin_err()
{
  ALOG_TRACE;
  MediaInputIntfPtr result = nullptr;

  do {
    ALOG_BREAK_IF(videoin_cnt_ >= kVideoInputMaxNum);

    int i = 0;
    for (; i < kVideoInputMaxNum; ++i) {
      if (videoin_array_[i] == nullptr)
        break;
    }
    string name = videoin_name[kVideoInputImage] + std::to_string(i);
    result = std::make_shared<MediaInputImplImage>();
    ALOG_BREAK_IF(result == nullptr);
    videoin_array_[i] = result;
    videoin_cnt_++;

    result->id_ = i;
    result->name_ = name;
    ALOG_BREAK_IF(0 != result->init());

    GstElement *new_bin = result->get_bin();
    ALOG_BREAK_IF(!new_bin);
    gst_bin_add(GST_BIN(videoin_bin_), new_bin);

    ALOGD("MediaInputModule create %s success", result->name_.c_str());
  } while(0);

  return result;
}

GstPad* MediaInputModule::get_request_pad(const MediaInputIntfPtr &ptr, bool is_video)
{
  GstPad *new_pad = nullptr;

  do {
    if (is_video) {
      new_pad = create_video_src_pad(ptr);
      // if (!prober_) {
      //   prober_ = std::make_shared<IPadProber>(
      //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, &deffunc_videoframe_info);
      // }
    } else {
      // TODO
    }
  } while(0);

  return new_pad;
}

void MediaInputModule::on_videoin_is_ready(MediaInputIntf *ptr)
{
  ALOG_TRACE;
  MediaInputIntfPtr input;

  do {
    for (auto it : videoin_array_) {
      if (!it) continue;
      if (ptr->id() == it->id()) {
        input = it;
        break;
      }
    }

    // MediaInputModule获取pad不会失败，而是切换到备用流
    // 如果MediaInputIntf未准备好，在这里重连
    // connect_selector(input);
    for (auto it : inpad_umap_) {
      it.second->reconnect(input);
    }
    // input->start();

    // create_video_src_pad(input);
    // TODO create_audio_src_pad
  } while(0);
}

void MediaInputModule::on_handle_bus_msg_error(GstBus *bus, GstMessage *msg)
{
  ALOG_TRACE;
  GstElement *elem;
  GstElementFactory *factory;

  do {
    for (auto input : videoin_array_) {
      if (!input) continue;

      GstElement *bin = input->get_bin();
      if (!bin) {
        ALOGD("Error to get bin from %s", input->name());
        continue;
      }
      if (gst_object_has_ancestor(msg->src, GST_OBJECT(bin))) {
        ALOGD("Detected error in input %s (id=%d)", input->name(), input->id());

        gst_element_set_state(bin, GST_STATE_PAUSED);
        // TODO 是否需要软重启？
        // ALOGD("Attempting soft-restart of input bin %s", input->name());
        // gst_element_set_state(bin, GST_STATE_PLAYING);

        // FIXME 备用流失败是否要考虑？
        // switch_selector(input, false);
        for (auto it : inpad_umap_) {
          it.second->sswitch(InputPadSwitch::kTypeStreamFallback1);
        }
      }
    }
  } while(0);
}

GstPad* MediaInputModule::create_video_src_pad(const MediaInputIntfPtr &ptr)
{
  ALOG_TRACE;
  InputPadSwitchPtr inpad;
  GstPad *ret = nullptr;

  do {
    lock_core_.lock();

    string new_pad_name = string("video_src_") + std::to_string(inpad_cnt_);

    inpad = std::make_shared<InputPadSwitch>(videoin_bin_, new_pad_name.c_str());
    ret = inpad->get_pad();

    inpad->connect(ptr, InputPadSwitch::kTypeStreamMain);
    inpad->connect(videoin_err_, InputPadSwitch::kTypeStreamFallback1);

    inpad_umap_.emplace(new_pad_name, std::move(inpad));
    inpad_cnt_++;
    lock_core_.unlock();
  } while(0);

  return ret;
}

} // namespace mmx