#include "media_input_module.h"

#include "media_input_impl_image.h"
#include "media_input_impl_hdmi.h"
#include "media_input_impl_uridb.h"
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
    connect_selector(input);

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

        gst_element_set_state(bin, GST_STATE_READY);
        // TODO
        // ALOGD("Attempting soft-restart of input bin %s", input->name());
        // gst_element_set_state(bin, GST_STATE_PLAYING);

        switch_selector(input, false);
      }
    }

    // ALOG_BREAK_IF(!GST_IS_ELEMENT(msg->src));
    // elem = GST_ELEMENT(msg->src);

    // factory = gst_element_get_factory(elem);
    // ALOG_BREAK_IF(!factory);

    // const gchar *fname = gst_plugin_feature_get_name(factory);
    // if (g_str_has_prefix(fname, "uridecodebin")) {

    // }

    // gst_object_unref(factory);
  } while(0);
}

GstPad* MediaInputModule::create_video_src_pad(const MediaInputIntfPtr &ptr)
{
  ALOG_TRACE;
  VideoRequestPad req;
  // bool is_main_stream_failed = true;

  do {
    lock_core_.lock();

    string new_pad_name = string("video_src_") + std::to_string(reqpad_cnt_);

    GstElement *new_selector = gst_element_factory_make("input-selector", nullptr);
    ALOG_BREAK_IF(!new_selector);
    gst_bin_add(GST_BIN(videoin_bin_), new_selector);

    // Link source0 -> input-selector
    GstPad *sink_pad0 = gst_element_get_request_pad(new_selector, "sink_%u");
    GstPad *src_pad0 = ptr->get_request_pad(true);
    if (src_pad0) {
      GstPadLinkReturn ret = gst_pad_link(src_pad0, sink_pad0);
      if (ret != GST_PAD_LINK_OK) {
        ALOGD("Failed to link src pad0 to sink pad0, ret:%d", ret);
      } else {
        req.indev_main_linked_ = true;
      }
      // gst_object_unref(src_pad0);
    } else {
      ALOGD("Failed to get request pad from %s", ptr->name());
    }
    if (sink_pad0) gst_object_unref(sink_pad0);

    // Link source1 -> input-selector
    GstPad *sink_pad1 = gst_element_get_request_pad(new_selector, "sink_%u");
    GstPad *src_pad1 = videoin_err_->get_request_pad(true);
    if (src_pad1) {
      if (gst_pad_link(src_pad1, sink_pad1) != GST_PAD_LINK_OK) {
        ALOGD("Failed to link src pad1 to sink pad1");
      }
      if (!req.indev_main_linked_) {
        g_object_set(new_selector, "active-pad", sink_pad1, NULL);
        ALOGD("Link fallback stream");
      }
      // gst_object_unref(src_pad1);
    } else {
      ALOGD("Failed to get request pad from %s", ptr->name());
    }
    if (sink_pad1) gst_object_unref(sink_pad1);

    // Create ghost pad to bin
    GstPad *ghost = nullptr;
    GstPad *selector_src_pad = gst_element_get_static_pad(new_selector, "src");
    if (selector_src_pad) {
      ghost = gst_ghost_pad_new(new_pad_name.c_str(), selector_src_pad);
      gst_element_add_pad(videoin_bin_, ghost);
      gst_object_unref(selector_src_pad);
    } else {
      ALOGD("Failed to get convert src pad");
    }

    req.inpad_ = ghost;
    req.inselect_ = new_selector;
    req.indev_list_ = {ptr, videoin_err_};
    req.indev_main_ = ptr->name();
    if (req.indev_main_linked_) {
      req.indev_curr_ = ptr->name();
    } else {
      req.indev_curr_ = videoin_err_->name();
    }

    reqpad_umap_.emplace(new_pad_name, req);
    reqpad_cnt_++;

    gst_element_sync_state_with_parent(new_selector);
    lock_core_.unlock();
  } while(0);

  return req.inpad_;
}

void MediaInputModule::connect_selector(const MediaInputIntfPtr &ptr)
{
  ALOG_TRACE;
  do {
    for (auto it : reqpad_umap_) {
      if (it.second.indev_main_ != ptr->name()) continue;
      if (it.second.indev_main_linked_) continue;

      GstPad *sink_pad0 = gst_element_get_static_pad(it.second.inselect_, "sink_0");
      ALOG_BREAK_IF(!sink_pad0);

      GstPad *src_pad0 = ptr->get_request_pad(true);
      if (src_pad0) {
        GstPadLinkReturn ret = gst_pad_link(src_pad0, sink_pad0);
        if (ret != GST_PAD_LINK_OK) {
          ALOGD("Failed to link src pad0 to sink pad0, ret:%d", ret);
        } else {
          it.second.indev_main_linked_ = true;
          g_object_set(G_OBJECT(it.second.inselect_), "active-pad", sink_pad0, NULL);
        }
        // gst_object_unref(src_pad0);
      } else {
        ALOGD("Failed to get request pad from %s", ptr->name());
      }
      if (sink_pad0) gst_object_unref(sink_pad0);
    }
  } while(0);
}

void MediaInputModule::switch_selector(const MediaInputIntfPtr &ptr, bool open)
{
  ALOG_TRACE;
  do {
    for (auto it : reqpad_umap_) {
      if (it.second.indev_main_ != ptr->name()) continue;

      bool is_main_stream = (it.second.indev_main_ == it.second.indev_curr_);
      if (open && !is_main_stream) {
        GstPad *sink_pad0 = gst_element_get_static_pad(it.second.inselect_, "sink0");
        if (sink_pad0) {
          g_object_set(G_OBJECT(it.second.inselect_), "active-pad", sink_pad0, NULL);
          gst_object_unref(sink_pad0);
        }
      } else if (!open && is_main_stream) {
        GstPad *sink_pad1 = gst_element_get_static_pad(it.second.inselect_, "sink1");
        if (sink_pad1) {
          g_object_set(G_OBJECT(it.second.inselect_), "active-pad", sink_pad1, NULL);
          gst_object_unref(sink_pad1);
        }
      }
    }
  } while(0);
}

} // namespace mmx