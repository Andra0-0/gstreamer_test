#include "media_input_impl_image.h"

#include <sstream>

#include "debug.h"

namespace mmx {

MediaInputImplImage::MediaInputImplImage()
{

}

MediaInputImplImage::~MediaInputImplImage()
{

}

// gint MediaInputImplImage::init()
// {
//   gint ret = -1;

//   do {
//     std::lock_guard<mutex> _l(lock_);
//     ALOG_BREAK_IF(uri_.empty());

//     // source_ = gst_element_factory_make("filesrc", "InputImageSource");
//     // ALOG_BREAK_IF(!source_);
//     /*Debug*/
//     source_ = gst_element_factory_make("videotestsrc", "InputImageSource");
//     ALOG_BREAK_IF(!source_);

//     // parser_ = gst_element_factory_make("jpegparse", "InputImageParse");
//     // ALOG_BREAK_IF(!parser_);

//     // decoder_ = gst_element_factory_make("mppjpegdec", "InputImageDecoder");
//     // ALOG_BREAK_IF(!decoder_);

//     convert_ = gst_element_factory_make("videoconvert", "InputImageConvert");
//     ALOG_BREAK_IF(!convert_);

//     videorate_ = gst_element_factory_make("videorate", "InputImageRate");
//     ALOG_BREAK_IF(!videorate_);

//     srccaps_ = gst_element_factory_make("capsfilter", "InputImageCaps");
//     ALOG_BREAK_IF(!srccaps_);

//     // imagefreeze_ = gst_element_factory_make("imagefreeze", "InputImageFreeze");
//     // ALOG_BREAK_IF(!imagefreeze_);

//     tee_ = gst_element_factory_make("tee", "InputImageTee");
//     ALOG_BREAK_IF(!tee_);

//     // g_object_set(G_OBJECT(source_),
//     //         "location", uri_.c_str(),
//     //         NULL);
//     /*Debug*/
//     g_object_set(G_OBJECT(source_),
//             "pattern", 1,
//             NULL);
//     // g_object_set(G_OBJECT(decoder_),
//     //         "format", 11, // 11-RGBA 16-BGR 23-NV12
//     //         "width", width_,
//     //         "height", height_,
//     //         "dma-feature", TRUE,
//     //         NULL);
//     // g_object_set(G_OBJECT(imagefreeze_),
//     //         "num-buffers", 5,
//     //         NULL);
//     GstCaps *caps = gst_caps_new_simple("video/x-raw",
//             // "format", G_TYPE_STRING, "RGBA",
//             // "width", G_TYPE_INT, 1920,
//             // "height", G_TYPE_INT, 1080,
//             // "framerate", GST_TYPE_FRACTION, 3, 1, // 3fps
//             NULL);
//     if (caps) {
//       g_object_set(G_OBJECT(srccaps_), "caps", caps, NULL);
//       gst_caps_unref(caps);
//     } else {
//       ALOGD("Failed to create caps for capsfilter");
//     }

//     gst_bin_add_many(GST_BIN(bin_), source_, /*parser_, decoder_,*/
//             convert_, /*imagefreeze_,*/ videorate_, srccaps_, tee_, NULL);

//     // FIXME: imagefreeze output too much video frame! (30000fps)
//     if (!gst_element_link_many(source_, /*parser_, decoder_,*/ convert_,
//             /*imagefreeze_,*/ videorate_, srccaps_, tee_, NULL)) {
//       ALOGD("Failed to link elements");
//     }

//     // if (!prober_) {
//     //   GstPad *new_pad = gst_element_get_static_pad(imagefreeze_, "src");
//     //   prober_ = std::make_shared<mmx::IPadProber>(
//     //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, mmx::deffunc_videoframe_info);
//     //   gst_object_unref(new_pad);
//     // }

//     state_ = kStreamStateReady;
//     ret = 0;
//   } while(0);

//   return ret;
// }

gint MediaInputImplImage::init()
{
  gint ret = -1;

  do {
    std::lock_guard<mutex> _l(lock_);
    ALOG_BREAK_IF(uri_.empty());

    source_ = gst_element_factory_make("filesrc", "InputImageSource");
    ALOG_BREAK_IF(!source_);

    parser_ = gst_element_factory_make("jpegparse", "InputImageParse");
    ALOG_BREAK_IF(!parser_);

    decoder_ = gst_element_factory_make("mppjpegdec", "InputImageDecoder");
    ALOG_BREAK_IF(!decoder_);

    convert_ = gst_element_factory_make("videoconvert", "InputImageConvert");
    ALOG_BREAK_IF(!convert_);

    videorate_ = gst_element_factory_make("videorate", "InputImageRate");
    ALOG_BREAK_IF(!videorate_);

    srccaps_ = gst_element_factory_make("capsfilter", "InputImageCaps");
    ALOG_BREAK_IF(!srccaps_);

    imagefreeze_ = gst_element_factory_make("imagefreeze", "InputImageFreeze");
    ALOG_BREAK_IF(!imagefreeze_);

    tee_ = gst_element_factory_make("tee", "InputImageTee");
    ALOG_BREAK_IF(!tee_);

    g_object_set(G_OBJECT(source_),
            "location", uri_.c_str(),
            NULL);
    g_object_set(G_OBJECT(decoder_),
            "format", 23, // 11-RGBA 16-BGR 23-NV12
            "width", 640,// width_,
            "height", 360, // height_,
            "dma-feature", TRUE,
            NULL);
    g_object_set(G_OBJECT(imagefreeze_),
            "is-live", TRUE,
            NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            // "width", G_TYPE_INT, 1920,
            // "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 3, 1, // 3fps
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(srccaps_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    gst_bin_add_many(GST_BIN(bin_), source_, parser_, decoder_,
            convert_, imagefreeze_, videorate_, srccaps_, tee_, NULL);

    if (!gst_element_link_many(source_, parser_, decoder_, convert_,
            imagefreeze_, videorate_, srccaps_, tee_, NULL)) {
      ALOGD("Failed to link elements");
    }

    // if (!prober_) {
    //   GstPad *new_pad = gst_element_get_static_pad(imagefreeze_, "src");
    //   prober_ = std::make_shared<mmx::IPadProbeVideoInfo>("imagefreeze", new_pad);
    //   gst_object_unref(new_pad);
    // }

    state_ = kStreamStateReady;
    ret = 0;
  } while(0);

  return ret;
}

gint MediaInputImplImage::deinit()
{
  do {
    state_ = kStreamStateInvalid;
  } while(0);

  return 0;
}

gint MediaInputImplImage::start()
{
  GstStateChangeReturn ret;

  do {
    state_ = kStreamStatePlaying;

    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PLAYING);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);
  } while(0);

  return 0;
}

gint MediaInputImplImage::pause()
{
  GstStateChangeReturn ret;

  do {
    state_ = kStreamStatePause;

    ret = gst_element_set_state(GST_ELEMENT(bin_), GST_STATE_PAUSED);
    ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);
  } while(0);

  return 0;
}

string MediaInputImplImage::get_info()
{
  std::ostringstream oss;
  GstState state, pending;

  oss << "\n\t============ get_info ============\n"
      << "\tClass: MediaInputImplImage\n"
      << "\tID: " << id_ << "\n"
      << "\tName: " << name_ << "\n";

  if (bin_) {
    gst_element_get_state(bin_, &state, &pending, 0);
    oss << "\tBinState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }
  if (source_) {
    gst_element_get_state(source_, &state, &pending, 0);
    oss << "\tFilesrcState cur:" << gst_element_state_get_name(state)
        << " pending:" << gst_element_state_get_name(pending) << "\n";
  }

  oss << "\t==================================";

  return oss.str();
}

GstPad* MediaInputImplImage::get_request_pad(const ReqPadInfo &info)
{
  GstPad *pad = nullptr;

  do {
    std::lock_guard<mutex> _l(lock_);
    if (info.is_video_) {
      pad = create_video_src_pad();
    } else {
      ALOGD("MediaInputImplImage couldn't request audio src pad");
    }
  } while(0);

  return pad;
}

gint MediaInputImplImage::set_property(const IMessagePtr &msg)
{
  do {
    if (msg->what() == IMSG_IMAGE_SETPROPERTY) {
      
    } else {
      ALOGD("Unsupported message what %u", msg->what());
    }
  } while(0);

  return 0;
}

void MediaInputImplImage::handle_bus_msg_error(GstBus *bus, GstMessage *msg)
{

}

// GstPad* MediaInputImplImage::create_video_src_pad()
// {
//   GstElement *new_queue;
//   GstPad *new_pad = nullptr;

//   do {
//     ALOG_BREAK_IF(state_ == kStreamStateInvalid);

//     new_queue = gst_element_factory_make("queue", nullptr);
//     ALOG_BREAK_IF(!new_queue);
//     gst_bin_add(GST_BIN(bin_), new_queue);

//     g_object_set(G_OBJECT(new_queue),
//             "max-size-buffers", 1,
//             "max-size-bytes", 0,
//             "max-size-time", 0,
//             "leaky", 0, // 0-no 1-upstream 2-downstream
//             NULL);

//     GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
//     GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
//     if (gst_pad_link(tee_src_pad, queue_sink_pad) != GST_PAD_LINK_OK) {
//       ALOGD("Failed to link tee src pad to queue sink pad");
//     }
//     gst_object_unref(tee_src_pad);
//     gst_object_unref(queue_sink_pad);

//     GstPad *queue_src_pad = gst_element_get_static_pad(new_queue, "src");
//     if (queue_src_pad) {
//       new_pad = vpad_mngr_->add_pad(queue_src_pad);
//       gst_object_unref(queue_src_pad);
//     } else {
//       ALOGD("Failed to get src pad");
//     }
//     // if (!prober_) {
//     //   prober_ = std::make_shared<mmx::IPadProber>(
//     //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, mmx::deffunc_videoframe_info);
//     // }

//     gst_element_sync_state_with_parent(new_queue);
//   } while(0);

//   return new_pad;
// }
GstPad* MediaInputImplImage::create_video_src_pad()
{
  GstElement *new_queue;
  GstElement *new_scale;
  GstElement *new_rate;
  GstElement *new_caps;
  GstPad *new_pad = nullptr;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_queue = gst_element_factory_make("queue", nullptr);
    ALOG_BREAK_IF(!new_queue);

    // rga_cvt = gst_element_factory_make("videoconvert", nullptr);
    // ALOG_BREAK_IF(!rga_cvt);

    new_scale = gst_element_factory_make("videoscale", nullptr);
    ALOG_BREAK_IF(!new_scale);

    new_rate = gst_element_factory_make("videorate", nullptr);
    ALOG_BREAK_IF(!new_rate);

    new_caps = gst_element_factory_make("capsfilter", nullptr);
    ALOG_BREAK_IF(!new_caps);

    gst_bin_add_many(GST_BIN(bin_), new_queue, new_scale, new_rate, new_caps, NULL);
    if (!gst_element_link_many(new_queue, new_scale, new_rate, new_caps, NULL)) {
      ALOGD("Failed to link queue -> new_scale -> new_rate -> new_caps");
      // return nullptr;
    }

    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            // "format", G_TYPE_STRING, "RGBA",
            // "width", G_TYPE_INT, 1920,
            // "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 30, 1, // 30fps
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(new_caps), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }
    // FIXME 这里max-size-buffers必须设为1，否则大量视频帧输出会导致异常
    g_object_set(G_OBJECT(new_queue),
            "max-size-buffers", 1,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 2, // 0-no 1-upstream 2-downstream
            NULL);

    GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
    GstPad *queue_sink_pad = gst_element_get_static_pad(new_queue, "sink");
    GstPadLinkReturn ret = gst_pad_link(tee_src_pad, queue_sink_pad);
    if (ret != GST_PAD_LINK_OK) {
      ALOGD("Failed to link tee src pad to queue sink pad, ret:%d", ret);
    }
    gst_object_unref(tee_src_pad);
    gst_object_unref(queue_sink_pad);

    GstPad *caps_src_pad = gst_element_get_static_pad(new_caps, "src");
    if (caps_src_pad) {
      new_pad = vpad_mngr_->add_pad(caps_src_pad);
      gst_object_unref(caps_src_pad);
    } else {
      ALOGD("Failed to get src pad");
    }
    // if (!prober_) {
    //   prober_ = std::make_shared<IPadProbeVideoInfo>(name_, new_pad);
    // }

    gst_element_sync_state_with_parent(new_queue);
  } while(0);

  return new_pad;
}

}