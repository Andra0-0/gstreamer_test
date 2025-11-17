#ifndef INPUT_IMPL_V4L2_H
#define INPUT_IMPL_V4L2_H

#include "input_intf.h"

#include <string>

class InputImplV4L2 : public InputIntf {
public:
  InputImplV4L2(const std::string &device, int io_mode);

  ~InputImplV4L2();

  GstElement* bin() override { return bin_; }

  GstPad* src_pad() override;

private:
  GstElement *bin_;
  GstElement *source_;
  GstElement *capsfilter_;
  GstElement *convert_;
  GstElement *queue_;
};

#endif // INPUT_IMPL_V4L2_H