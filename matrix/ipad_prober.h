#ifndef MATRIX_IPAD_PROBER_H
#define MATRIX_IPAD_PROBER_H

#include <gst/gst.h>
#include <future>
#include <memory>

#include "noncopyable.h"

namespace mmx {

class IPadProber;

using IPadProberPtr = std::shared_ptr<IPadProber>;

GstPadProbeReturn deffunc_handle_event(GstPadProbeInfo *info);
GstPadProbeReturn deffunc_videoframe_info(GstPadProbeInfo *info);

class IPadProber : public NonCopyable {
public:
  using IPadHandler = std::function<GstPadProbeReturn(GstPadProbeInfo*)>;

  IPadProber(GstPad *pad, GstPadProbeType mask, IPadHandler handler);

  ~IPadProber();

protected:
  static GstPadProbeReturn on_pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer data);

private:
  GstPad *pad_; // IPadProber hold pointer and add reference
  guint probe_id_; // Probe ID for destructor
  IPadHandler handler_;
};

}

#endif // MATRIX_IPAD_PROBER_H