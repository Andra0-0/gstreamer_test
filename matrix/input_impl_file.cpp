#include "input_impl_file.h"

#include "debug.h"

static gint file_location_check(const std::string &location)
{
  if (location.empty()) {
    return -1;
  }
  return 0;
}

InputImplFile::InputImplFile(const std::string &location)
  : bin_(nullptr)
  , source_(nullptr)
  , decoder_(nullptr)
  , convert_(nullptr)
{
  do {
    bin_ = gst_bin_new("InputFileBin");
    ALOG_BREAK_IF(!bin_);

    convert_ = gst_element_factory_make("videoconvert", "InputFileConvert");
    ALOG_BREAK_IF(!convert_);

    source_ = gst_element_factory_make("filesrc", NULL);
    decoder_ = gst_element_factory_make("decodebin", NULL);
    queue_ = gst_element_factory_make("queue", NULL);
    ALOG_BREAK_IF(!source_ || !decoder_ || !queue_);

    g_object_set(G_OBJECT(source_),
            "location", location.c_str(), NULL);
    g_object_set(G_OBJECT(queue_),
            "max-size-buffers", 5,
            "max-size-bytes", 0,
            "max-size-time", 0,
            "leaky", 2, NULL);

    gst_bin_add_many(GST_BIN(bin_), source_, decoder_, convert_, queue_, NULL);
    if (!gst_element_link(source_, decoder_)) {
      ALOGD("Failed to link filesrc -> decodebin");
    }
    /* decodebin will produce pads dynamically; connect callback to link to queue sink */
    g_signal_connect(decoder_, "pad-added", G_CALLBACK(InputImplFile::onDecodebinPadAdded), this);

    GstPad *pad = gst_element_get_static_pad(queue_, "src");
    if (pad) {
      GstPad *ghost = gst_ghost_pad_new("src", pad);
      gst_element_add_pad(bin_, ghost);
      gst_object_unref(pad);
    } else {
      ALOGD("Failed to get queue src pad");
    }
  } while(0);
}

InputImplFile::~InputImplFile()
{
  if (bin_) {
    gst_object_unref(bin_);
  }
}

GstPad* InputImplFile::src_pad()
{
  if (!bin_) {
    return nullptr;
  }
  return gst_element_get_static_pad(bin_, "src");
}

void InputImplFile::onDecodebinPadAdded(GstElement * /*decodebin*/, GstPad *pad, gpointer data)
{
  InputImplFile *self = static_cast<InputImplFile*>(data);
  if (!self) {
    return;
  }

  GstCaps *caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    return;
  }

  GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);

  if (g_str_has_prefix(name, "video")) {
    GstPad *sink_pad = gst_element_get_static_pad(self->convert_, "sink");
    if (gst_pad_is_linked(sink_pad)) {
      gst_object_unref(sink_pad);
      gst_caps_unref(caps);
      return;
    }

    GstPadLinkReturn ret = gst_pad_link(pad, sink_pad);
    if (ret != GST_PAD_LINK_OK) {
      ALOGD("Failed to link decodebin pad to convert_ sink");
    }
    gst_object_unref(sink_pad);
  }

  gst_caps_unref(caps);
}