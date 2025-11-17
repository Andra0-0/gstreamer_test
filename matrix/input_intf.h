#ifndef _INPUT_INTF_H_
#define _INPUT_INTF_H_

#include <string>
#include <memory>

extern "C" {
#include <gst/gst.h>
#include <glib.h>
}

class InputIntf;
using InputIntfPtr = std::shared_ptr<InputIntf>;

/**
 * Input device interface class
 */
class InputIntf {
public:
  // InputIntf();

  virtual ~InputIntf() {}

  // Return GstBin that should be added to the pipeline
  virtual GstElement* bin() = 0;

  // Return the source pad (video) from the bin to link to compositor
  virtual GstPad* src_pad() = 0;

private:

};

#endif // _INPUT_INTF_H_