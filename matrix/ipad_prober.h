#ifndef MATRIX_IPAD_PROBER_H
#define MATRIX_IPAD_PROBER_H

#include <gst/gst.h>

#include "noncopyable.h"

namespace mmx {

class IPadProber : public NonCopyable {
public:
  IPadProber(GstPad *pad, GstPadProbeType mask);

  static GstPadProbeReturn on_pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer data);
};

}

#endif // MATRIX_IPAD_PROBER_H