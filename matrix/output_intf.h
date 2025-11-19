#ifndef _OUTPUT_INTF_H_
#define _OUTPUT_INTF_H_

#include <string>
#include <memory>

extern "C" {
#include <gst/gst.h>
#include <glib.h>
}

class OutputIntf;
using OutputIntfPtr = std::shared_ptr<OutputIntf>;

/**
 * Output device interface class
 */
class OutputIntf {
public:
  // OutputIntf();

  virtual ~OutputIntf() {}

  // Return GstBin that should be added to the pipeline
  virtual GstElement* bin() = 0;

  // Return the sink pad (video) from the bin to link to compositor
  virtual GstPad* sink_pad() = 0;

private:
};

#endif // _OUTPUT_INTF_H_