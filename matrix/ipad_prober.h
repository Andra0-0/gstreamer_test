#ifndef MATRIX_IPAD_PROBER_H
#define MATRIX_IPAD_PROBER_H

#include <gst/gst.h>
#include <future>
#include <memory>
#include <string>

#include "noncopyable.h"

namespace mmx {

class IPadProber;
class IPadProbeVideoInfo;

using std::string;
using IPadProberPtr = std::shared_ptr<IPadProber>;
using IPadProbeVideoInfoPtr = std::shared_ptr<IPadProbeVideoInfo>;

GstPadProbeReturn deffunc_handle_event(GstPadProbeInfo *info);
GstPadProbeReturn deffunc_videoframe_info(GstPadProbeInfo *info);

class IPadProber : public NonCopyable {
public:
  using IPadHandler = std::function<GstPadProbeReturn(GstPadProbeInfo*)>;

  IPadProber(GstPad *pad, GstPadProbeType mask, IPadHandler handler);

  ~IPadProber();

  GstPad* get_pad() const { return pad_; }

protected:
  static GstPadProbeReturn on_pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer data);

private:
  GstPad *pad_; // IPadProber hold pointer and add reference
  guint probe_id_; // Probe ID for destructor
  IPadHandler handler_;
};

/**
 * 计算视频帧率信息
 */
class IPadProbeVideoInfo {
public:
  IPadProbeVideoInfo(string name, GstPad *pad);
  ~IPadProbeVideoInfo();
  GstPadProbeReturn report_video_rate(GstPadProbeInfo *info);
  IPadProbeVideoInfo& operator=(const IPadProbeVideoInfo &other);

private:
  IPadProberPtr prober_;
  GstPadProbeType mask_;
  string name_;
  double theoretical_fps_;
  timeval timestamp_;
  bool first_frame_;
  guint frame_cnt_;
};

}

#endif // MATRIX_IPAD_PROBER_H