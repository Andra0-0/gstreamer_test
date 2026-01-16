#include "media_input_impl_uridb.h"

#include <sstream>
#include <functional>

#include "debug.h"

namespace mmx {

MediaInputImplUridb::MediaInputImplUridb()
  : source_(nullptr)
  , video_cvt_(nullptr)
  , video_scale_(nullptr)
  , video_caps_(nullptr)
  , video_tee_(nullptr)
  , audio_tee_(nullptr)
  , video_linked_(false)
{

}

MediaInputImplUridb::~MediaInputImplUridb()
{

}

gint MediaInputImplUridb::init()
{
  ALOG_TRACE;
  gint ret = -1;

  do {
    ALOG_BREAK_IF(uri_.empty());

    source_ = gst_element_factory_make("uridecodebin", "InputUriSource");
    ALOG_BREAK_IF(!source_);

    video_cvt_ = gst_element_factory_make("videoconvert", "UriVideoConvert");
    ALOG_BREAK_IF(!video_cvt_);

    video_scale_ = gst_element_factory_make("videoscale", "UriVideoScale");
    ALOG_BREAK_IF(!video_scale_);

    video_caps_ = gst_element_factory_make("capsfilter", "UriVideoCaps");
    ALOG_BREAK_IF(!video_caps_);

    video_tee_ = gst_element_factory_make("tee", "UriVideoTee");
    ALOG_BREAK_IF(!video_tee_);

    // fakesink_ = gst_element_factory_make("fakesink", "UriFakeSink");
    // ALOG_BREAK_IF(!fakesink_);

    // fakequeue_ = gst_element_factory_make("queue", "UriFakeQueue");
    // ALOG_BREAK_IF(!fakequeue_);

    g_object_set(G_OBJECT(source_),
            "uri", uri_.c_str(),
            // "uri", "v4l2:///dev/video0",
            // "uri", "file:///home/cat/test/test.mp4",
            // "uri", "rtsp://192.168.3.137:8554/h264ESVideoTest",
            // "uri", "rtsp://192.168.3.137:8554/mpeg2TransportStreamTest",
            // "uri", "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_60fps_normal.mp4",
            NULL);
    // g_object_set(G_OBJECT(fakesink_),
    //         "enable-last-sample", FALSE,
    //         "sync", FALSE,
    //         "async", FALSE,
    //         NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            "width", G_TYPE_INT, width_,
            "height", G_TYPE_INT, height_,
            // "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(video_caps_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    gst_bin_add_many(GST_BIN(bin_), source_,
            video_cvt_, video_scale_, video_caps_, video_tee_,
            /*fakequeue_, fakesink_,*/ NULL);

    g_signal_connect(source_, "pad-added", G_CALLBACK(on_uridb_pad_added), this);
    g_signal_connect(source_, "no-more-pads", G_CALLBACK(on_uridb_no_more_pads), this);
    g_signal_connect(source_, "element-added", G_CALLBACK(on_uridb_element_added), this);
    g_signal_connect(source_, "unknown-type", G_CALLBACK(on_uridb_unknown_type), this);

    ret = 0;
  } while(0);

  return ret;
}

gint MediaInputImplUridb::deinit()
{
  do {
    state_ = kStreamStateInvalid;

    if (source_) {
      gst_element_set_state(source_, GST_STATE_NULL);
    }
  } while(0);

  return 0;
}

gint MediaInputImplUridb::start()
{
  GstStateChangeReturn ret;

  do {
    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PLAYING);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

    ret = gst_element_set_state(GST_ELEMENT(source_), GST_STATE_READY);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

    ret = gst_element_set_state(GST_ELEMENT(source_), GST_STATE_PLAYING);
    if (ret != GST_STATE_CHANGE_SUCCESS) {
      ALOGD("MediaInputImplUridb state change not success, ret:%d", ret);
    }

    state_ = kStreamStatePlaying;
  } while(0);

  return 0;
}

gint MediaInputImplUridb::pause()
{
  GstStateChangeReturn ret;

  do {
    ALOG_BREAK_IF(state_ != kStreamStatePlaying);

    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PAUSED);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

    state_ = kStreamStatePause;
  } while(0);

  return ret;
}

string MediaInputImplUridb::get_info()
{
  std::ostringstream oss;
  GstState state, pending;

  oss << "\n\t============ get_info ============\n"
      << "\tClass: MediaInputImplUridb\n"
      << "\tID: " << id_ << "\n"
      << "\tName: " << name_ << "\n";

  if (bin_) {
    gst_element_get_state(bin_, &state, &pending, 0);
    oss << "\tBinState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }
  if (source_) {
    gst_element_get_state(source_, &state, &pending, 0);
    oss << "\tUridbState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }
  if (video_tee_) {
    gst_element_get_state(video_tee_, &state, &pending, 0);
    oss << "\tvideo_tee_ cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }

  oss << "\t==================================";

  return oss.str();
}

GstPad* MediaInputImplUridb::get_request_pad(const ReqPadInfo &info)
{
  ALOG_TRACE;
  GstPad *pad = nullptr;

  do {
    // lock_.lock();
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    if (info.is_video_) {
      pad = inter_get_video_pad();
    } else {
      // TODO
    }
    // lock_.unlock();
  } while(0);

  return pad;
}

gint MediaInputImplUridb::set_property(const IMessagePtr &msg)
{
  do {
    if (msg->what() == IMSG_URIDB_SETPROPERTY) {
      int width = 0, height = 0;
      string uri;
      ALOG_BREAK_IF(!msg->find_string("uri", uri));
      ALOG_BREAK_IF(!msg->find_int32("width", width));
      ALOG_BREAK_IF(!msg->find_int32("height", height));

    } else {
      ALOGD("Unsupported message what %u", msg->what());
    }
  } while(0);

  return 0;
}

void MediaInputImplUridb::handle_bus_msg_error(GstBus *bus, GstMessage *msg)
{
  ALOG_TRACE;
  do {
    state_ = kStreamStateInvalid;
    gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PAUSED);
  } while(0);
}

void MediaInputImplUridb::on_uridb_pad_added(GstElement *obj, GstPad *pad, void *data)
{
  ALOG_TRACE;
  ALOGD("Recevied new pad '%s' from '%s", GST_PAD_NAME(pad), GST_ELEMENT_NAME(obj));

  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);
  GstCaps *pad_caps;
  GstStructure *pad_struct;
  const gchar *pad_type;

  do {
    self->state_ = kStreamStateReady;

    pad_caps = gst_pad_get_current_caps(pad);
    ALOG_BREAK_IF(!pad_caps);

    pad_struct = gst_caps_get_structure(pad_caps, 0);
    pad_type = gst_structure_get_name(pad_struct);

    if (g_str_has_prefix(pad_type, "video/")) {
      if (!self->video_linked_) {
        // 链接视频处理流程
        self->inter_handle_video_pad(pad);
        // 回调，创建queue连接tee，重连input-selector
        // MediaInputModule::instance()->on_indev_video_pad_added(self);
        self->signal_indev_video_pad_added(self);
        self->video_linked_ = true;
      } else {
        ALOGD("Already linked video stream");
      }
    } else if (g_str_has_prefix(pad_type, "audio/")) {
      // TODO
      ALOGD("TODO add audio pad process");
    }
  } while(0);
}

void MediaInputImplUridb::on_uridb_no_more_pads(GstElement *obj, void *data)
{
  ALOG_TRACE;
  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);

  do {
    // self->lock_.lock();
    // self->state_ = kStreamStateReady;
    // self->lock_.unlock();

    // MediaInputModule::instance()->on_indev_no_more_pads(self);
    self->signal_indev_no_more_pads(self);
  } while(0);
}

void MediaInputImplUridb::on_uridb_element_added(GstElement *obj, GstElement *elem, void *data)
{
  // ALOG_TRACE;
  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);
  GstElementFactory *factory = gst_element_get_factory(elem);

  if (factory) {
    if (g_strcmp0(gst_plugin_feature_get_name(factory), "mppvideodec") == 0) {
      g_object_set(elem,
              "format", 23, // RGBA:11 RGB:15 BGR:16 NV12:23
              "width", self->width_,
              "height", self->height_,
              "dma-feature", TRUE,
              // "disable-dts", TRUE, // try to disable dts, might help mpp generate pts
              // "max-lateness", 200000000, // 200ms
              NULL);
    } else if (g_strcmp0(gst_plugin_feature_get_name(factory), "rtspsrc") == 0) {
      g_object_set(elem,
              "latency", 200, // 200ms 延迟缓冲
              // "protocols", 4, // 4-tcp 7-tcp/udp/udp-mcast
              // "retry", 3, // 重试次数
              // "timeout", 10, // 超时时间 10 秒
              // "tcp-timeout", 20000000, // TCP 超时 20 秒 (微秒)
              // "drop-on-latency", TRUE, // 延迟时丢帧
              NULL);
    }
  }
  if (GST_IS_BIN(elem)) {
    g_signal_connect(elem, "element-added", G_CALLBACK(on_uridb_element_added), data);
  }
}

void MediaInputImplUridb::on_uridb_unknown_type(GstElement *obj, GstPad *pad, GstCaps caps, void *data)
{
  ALOG_TRACE;
}

/**
 * 创建新的src_pad和video queue到bin
 */
GstPad* MediaInputImplUridb::inter_get_video_pad()
{
  ALOG_TRACE;
  GstElement *new_queue;
  GstPad *new_pad;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_queue = gst_element_factory_make("queue", nullptr);
    ALOG_BREAK_IF(!new_queue);
    gst_bin_add(GST_BIN(bin_), new_queue);

    GstPad *tee_src_pad = gst_element_get_request_pad(video_tee_, "src_%u");
    GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
    if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link tee src pad to queue sink pad");
    }
    gst_object_unref(tee_src_pad);
    gst_object_unref(queue_sink_pad);

    GstPad *queue_src_pad = gst_element_get_static_pad(new_queue, "src");
    if (queue_src_pad) {
      new_pad = vpad_mngr_->add_pad(queue_src_pad);
      gst_object_unref(queue_src_pad);
    } else {
      ALOGD("Failed to get src pad");
    }

    // if (!prober_) {
    //   prober_ = std::make_shared<IPadProber>(
    //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, deffunc_videoframe_info);
    // }

    gst_element_sync_state_with_parent(new_queue);
    video_queue_.emplace(GST_PAD_NAME(new_pad), new_queue);
  } while(0);

  return new_pad;
}
// GstPad* MediaInputImplUridb::inter_get_video_pad()
// {
//   ALOG_TRACE;
//   GstElement *new_queue;
//   GstElement *new_scale;
//   GstElement *new_rate;
//   GstElement *new_caps;
//   GstPad *new_pad;

//   do {
//     ALOG_BREAK_IF(state_ == kStreamStateInvalid);

//     new_queue = gst_element_factory_make("queue", nullptr);
//     ALOG_BREAK_IF(!new_queue);

//     new_scale = gst_element_factory_make("videoscale", nullptr);
//     ALOG_BREAK_IF(!new_scale);

//     new_rate = gst_element_factory_make("videorate", nullptr);
//     ALOG_BREAK_IF(!new_rate);

//     new_caps = gst_element_factory_make("capsfilter", nullptr);
//     ALOG_BREAK_IF(!new_caps);

//     gst_bin_add_many(GST_BIN(bin_), new_queue, new_scale, new_rate, new_caps, NULL);
//     if (!gst_element_link_many(new_queue, new_scale, new_rate, new_caps, NULL)) {
//       ALOGD("Failed to link queue -> new_scale -> new_rate -> new_caps");
//       // return nullptr;
//     }

//     GstCaps *caps = gst_caps_new_simple("video/x-raw",
//             // "format", G_TYPE_STRING, "RGBA",
//             // "width", G_TYPE_INT, 1920,
//             // "height", G_TYPE_INT, 1080,
//             "framerate", GST_TYPE_FRACTION, 30, 1, // 30fps
//             NULL);
//     if (caps) {
//       g_object_set(G_OBJECT(new_caps), "caps", caps, NULL);
//       gst_caps_unref(caps);
//     } else {
//       ALOGD("Failed to create caps for capsfilter");
//     }
//     // g_object_set(G_OBJECT(new_queue),
//     //         // "max-size-buffers", 3,
//     //         // "max-size-bytes", 0,
//     //         // "max-size-time", 0,
//     //         "leaky", 2, // 0-no 1-upstream 2-downstream
//     //         NULL);

//     GstPad *tee_src_pad = gst_element_get_request_pad(video_tee_, "src_%u");
//     GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
//     if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
//       ALOGD("Failed to link tee src pad to queue sink pad");
//     }
//     gst_object_unref(tee_src_pad);
//     gst_object_unref(queue_sink_pad);

//     GstPad *caps_src_pad = gst_element_get_static_pad(new_caps, "src");
//     if (caps_src_pad) {
//       new_pad = vpad_mngr_->add_pad(caps_src_pad);
//       gst_object_unref(caps_src_pad);
//     } else {
//       ALOGD("Failed to get src pad");
//     }

//     // if (!prober_) {
//     //   prober_ = std::make_shared<IPadProber>(
//     //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, deffunc_videoframe_info);
//     // }

//     gst_element_sync_state_with_parent(new_queue);
//     video_queue_.emplace(GST_PAD_NAME(new_pad), new_queue);
//   } while(0);

//   return new_pad;
// }

/**
 * 创建视频流处理
 */
void MediaInputImplUridb::inter_handle_video_pad(GstPad *pad)
{
  do {
    if (!gst_element_link_many(video_cvt_, video_scale_, video_caps_, video_tee_,/* fakequeue_, fakesink_,*/ NULL)) {
      ALOGD("Failed to link elements");
    }

    GstPad *sink_pad = gst_element_get_static_pad(video_cvt_, "sink");
    if (gst_pad_is_linked(sink_pad)) {
      ALOGD("Error to link videoconvert, sink pad already linked");
      break;
    }
    if (gst_pad_link(pad, sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Error to link uridecodebin pad -> videoconvert");
    }
    if (!prober_) {
      IPadProber::IPadHandler handler;
      handler = std::bind(&mmx::MediaInputImplUridb::on_handle_end_of_stream,
              this, std::placeholders::_1);
      prober_ = std::make_shared<IPadProber>(sink_pad,
              GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, handler);
    }
    gst_object_unref(sink_pad);
  } while(0);
}

void MediaInputImplUridb::inter_handle_audio_pad(GstPad *pad)
{
  // TODO
}

/**
 * Capture uridecodebin eos signal
 */
GstPadProbeReturn MediaInputImplUridb::on_handle_end_of_stream(GstPadProbeInfo *info)
{
  if (GST_EVENT_TYPE(GST_PAD_PROBE_INFO_DATA(info)) == GST_EVENT_EOS) {
    signal_indev_end_of_stream(this);
    return GST_PAD_PROBE_DROP;
  }
  return GST_PAD_PROBE_OK;
}

}