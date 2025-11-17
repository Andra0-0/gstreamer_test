#ifndef INPUT_IMPL_FILE_H
#define INPUT_IMPL_FILE_H

#include "input_intf.h"

#include <string>

class InputImplFile : public InputIntf {
public:
  InputImplFile(const std::string &location);

  ~InputImplFile();

  GstElement* bin() override { return bin_; }

  GstPad* src_pad() override;

  static void onDecodebinPadAdded(GstElement *decodebin, GstPad *pad, gpointer data);

private:
  GstElement *bin_;
  GstElement *source_;
  GstElement *decoder_;
  GstElement *convert_;
  GstElement *queue_;
};

#endif // INPUT_IMPL_FILE_H