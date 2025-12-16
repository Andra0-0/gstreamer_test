#include "media_input_impl_image.h"

#include <sstream>

#include "debug.h"

namespace mmx {

MediaInputImplImage::MediaInputImplImage()
  : video_pad_cnt_(0)
{

}

MediaInputImplImage::~MediaInputImplImage()
{

}

gint MediaInputImplImage::init()
{
  gint ret = -1;

  do {
    std::lock_guard<mutex> _l(lock_);
    ALOG_BREAK_IF(uri_.empty());

    bin_ = gst_bin_new(name_.c_str());
    ALOG_BREAK_IF(!bin_);

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

    capsfilter_ = gst_element_factory_make("capsfilter", "InputImageCaps");
    ALOG_BREAK_IF(!capsfilter_);

    imagefreeze_ = gst_element_factory_make("imagefreeze", "InputImageFreeze");
    ALOG_BREAK_IF(!imagefreeze_);

    tee_ = gst_element_factory_make("tee", "InputImageTee");
    ALOG_BREAK_IF(!tee_);

    g_object_set(G_OBJECT(source_),
            "location", uri_.c_str(),
            NULL);
    g_object_set(G_OBJECT(decoder_),
            "format", 11, // 11-RGBA 16-BGR 23-NV12
            "width", width_,
            "height", height_,
            NULL);
    // g_object_set(G_OBJECT(imagefreeze_),
    //         "num-buffers", 5,
    //         NULL);
    GstCaps *caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            // "width", G_TYPE_INT, 1920,
            // "height", G_TYPE_INT, 1080,
            "framerate", GST_TYPE_FRACTION, 3, 1, // 3fps
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(capsfilter_), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      ALOGD("Failed to create caps for capsfilter");
    }

    gst_bin_add_many(GST_BIN(bin_), source_, parser_, decoder_,
            convert_, imagefreeze_, videorate_, capsfilter_, tee_, NULL);

    // FIXME: imagefreeze output too much video frame! (30000fps)
    if (!gst_element_link_many(source_, parser_, decoder_, convert_,
            imagefreeze_, videorate_, capsfilter_, tee_, NULL)) {
      ALOGD("Failed to link elements");
    }

    // if (!prober_) {
    //   GstPad *new_pad = gst_element_get_static_pad(imagefreeze_, "src");
    //   prober_ = std::make_shared<mmx::IPadProber>(
    //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, mmx::deffunc_videoframe_info);
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
  do {
    state_ = kStreamStatePlaying;
  } while(0);

  return 0;
}

gint MediaInputImplImage::pause()
{
  do {
    state_ = kStreamStatePause;
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

GstElement* MediaInputImplImage::get_bin()
{
  return bin_;
}

GstPad* MediaInputImplImage::get_request_pad(bool is_video)
{
  GstPad *pad = nullptr;

  do {
    std::lock_guard<mutex> _l(lock_);
    if (is_video) {
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

GstPad* MediaInputImplImage::create_video_src_pad()
{
  GstElement *new_queue;
  string new_pad_name;
  GstPad *new_pad = nullptr;

  do {
    ALOG_BREAK_IF(state_ == kStreamStateInvalid);

    new_pad_name = string("video_src_") + std::to_string(video_pad_cnt_);

    new_queue = gst_element_factory_make("queue", new_pad_name.c_str());
    ALOG_BREAK_IF(!new_queue);
    gst_bin_add(GST_BIN(bin_), new_queue);

    g_object_set(G_OBJECT(new_queue),
            "max-size-buffers", 1,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 0, // 0-no 1-upstream 2-downstream
            NULL);

    GstPad *tee_src_pad = gst_element_get_request_pad(tee_, "src_%u");
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
    // if (!prober_) {
    //   prober_ = std::make_shared<mmx::IPadProber>(
    //           new_pad, GST_PAD_PROBE_TYPE_BUFFER, mmx::deffunc_videoframe_info);
    // }

    video_pad_cnt_++;
    gst_element_sync_state_with_parent(new_queue);
  } while(0);

  return new_pad;
}

}