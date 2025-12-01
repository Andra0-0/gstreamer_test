#include "media_input_impl_uridb.h"

#include "media_input_module.h"
#include "debug.h"

namespace mmx {

MediaInputImplUridb::MediaInputImplUridb()
  : bin_(nullptr)
  , source_(nullptr)
  , video_cvt_(nullptr)
  , video_caps_(nullptr)
  , video_tee_(nullptr)
  , audio_tee_(nullptr)
  , video_pad_cnt_(0)
  , video_linked_(false)
{
  ALOG_TRACE;
}

MediaInputImplUridb::~MediaInputImplUridb()
{

}

gint MediaInputImplUridb::init()
{
  ALOG_TRACE;
  gint ret = -1;

  do {
    bin_ = gst_bin_new(name_.c_str());
    ALOG_BREAK_IF(!bin_);

    source_ = gst_element_factory_make("uridecodebin", "InputUriSource");
    ALOG_BREAK_IF(!source_);

    video_cvt_ = gst_element_factory_make("videoconvert", "UriVideoConvert");
    ALOG_BREAK_IF(!video_cvt_);

    video_caps_ = gst_element_factory_make("capsfilter", "UriVideoCaps");
    ALOG_BREAK_IF(!video_caps_);

    video_tee_ = gst_element_factory_make("tee", "UriVideoTee");
    ALOG_BREAK_IF(!video_tee_);

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, 1920,
            "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 30, 1,
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(video_caps_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    g_object_set(G_OBJECT(source_),
            "uri", "v4l2:///dev/video0",
            NULL);

    gst_bin_add_many(GST_BIN(bin_), source_, video_cvt_, video_caps_, video_tee_, NULL);

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
  } while(0);

  return 0;
}

gint MediaInputImplUridb::start()
{
  do {
    state_ = kStreamStatePlaying;
  } while(0);

  return 0;
}

gint MediaInputImplUridb::pause()
{
  do {
    state_ = kStreamStatePause;
  } while(0);

  return 0;
}

GstElement* MediaInputImplUridb::get_bin()
{
  return bin_;
}

GstPad* MediaInputImplUridb::get_request_pad(bool is_video)
{
  ALOG_TRACE;
  GstPad *pad = nullptr;

  do {
    // lock_.lock();
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    if (is_video) {
      pad = create_video_src_pad();
    } else {
      // TODO
    }
    // lock_.unlock();
  } while(0);

  return pad;
}

gint MediaInputImplUridb::set_property(const string &name, const MetaMessagePtr &msg)
{

}

void MediaInputImplUridb::on_uridb_pad_added(GstElement *obj, GstPad *pad, void *data)
{
  // TODO 这里逻辑有问题，输出流不能阻塞，可能需要将pad留到no more pads添加
  // 或者用fakesink
  ALOG_TRACE;
  ALOGD("Recevied new pad '%s' from '%s", GST_PAD_NAME(pad), GST_ELEMENT_NAME(obj));

  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);
  GstPad *sink_pad;
  GstCaps *pad_caps;
  GstStructure *pad_struct;
  const gchar *pad_type;

  do {
    pad_caps = gst_pad_get_current_caps(pad);
    ALOG_BREAK_IF(!pad_caps);

    pad_struct = gst_caps_get_structure(pad_caps, 0);
    pad_type = gst_structure_get_name(pad_struct);

    if (g_str_has_prefix(pad_type, "video/")) {
      if (!self->video_linked_) {
        self->create_video_process(pad);
        self->video_linked_ = true;
      } else {
        ALOGD("Already linked video stream");
      }
    } else if (g_str_has_prefix(pad_type, "audio/")) {
      // TODO
    }
  } while(0);
}

void MediaInputImplUridb::on_uridb_no_more_pads(GstElement *obj, void *data)
{
  ALOG_TRACE;
  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);

  do {
    // self->lock_.lock();
    self->state_ = kStreamStateReady;
    // self->lock_.unlock();

    MediaInputModule::instance()->on_videoin_is_ready(self);
  } while(0);
}

void MediaInputImplUridb::on_uridb_element_added(GstElement *obj, GstElement *elem, void *data)
{
  ALOG_TRACE;
  MediaInputImplUridb *const self = static_cast<MediaInputImplUridb*>(data);
  GstElementFactory *factory = gst_element_get_factory(elem);

  if (factory) {
    if (g_strcmp0(gst_plugin_feature_get_name(factory), "mppvideodec") == 0) {
      g_object_set(G_OBJECT(elem), "format", 11, "dma-feature", TRUE, NULL);
    } else if (g_strcmp0(gst_plugin_feature_get_name(factory), "rtspsrc") == 0) {
      g_object_set(elem, "latency", 200, NULL);
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
GstPad* MediaInputImplUridb::create_video_src_pad()
{
  ALOG_TRACE;
  GstElement *new_queue;
  string new_pad_name;
  GstPad *new_pad;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_pad_name = string("video_src_") + std::to_string(video_pad_cnt_);

    new_queue = gst_element_factory_make("queue", new_pad_name.c_str());
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
      new_pad = gst_ghost_pad_new(new_pad_name.c_str(), queue_src_pad);
      gst_element_add_pad(bin_, new_pad);
      gst_object_unref(queue_src_pad);
    } else {
      ALOGD("Failed to get src pad");
    }

    gst_element_sync_state_with_parent(new_queue);
    video_queue_.emplace(new_pad_name, new_queue);
    video_pad_cnt_++;
  } while(0);

  return new_pad;
}

/**
 * 创建视频流处理
 */
void MediaInputImplUridb::create_video_process(GstPad *pad)
{
  do {
    gst_element_sync_state_with_parent(video_cvt_);
    gst_element_sync_state_with_parent(video_caps_);
    gst_element_sync_state_with_parent(video_tee_);

    if (!gst_element_link_many(video_cvt_, video_caps_, video_tee_, NULL)) {
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
    gst_object_unref(sink_pad);
  } while(0);
}

void MediaInputImplUridb::create_audio_process(GstPad *pad)
{
  // TODO
}

}