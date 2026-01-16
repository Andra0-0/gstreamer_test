#ifndef OUTPUT_IMPL_KMS_H
#define OUTPUT_IMPL_KMS_H

#include "output_intf.h"
#include "ipad_prober.h"

#include <string>

extern "C" {
#include <xf86drm.h>
#include <xf86drmMode.h>
}

namespace mmx {

using std::string;

class OutputImplKms : public OutputIntf {
public:
  enum hdmi_tx_port {
    HDMI_TX_0 = 0,
    HDMI_TX_1,
    HDMI_TX_2,
  };
  struct DRM {
    DRM(const char *device);
    ~DRM();

    bool init();
    void deinit();

    int fd_;
    string device_;
    drmModeRes *res_;
    drmModePlaneRes *plane_res_;
  };

  explicit OutputImplKms(hdmi_tx_port port);

  ~OutputImplKms();

  GstElement* bin() override { return bin_; }

  GstPad* sink_pad() override;

  /**
   * 清空输出缓冲区
   */
  gint refresh() override;

private:
  GstElement *bin_;
  GstElement *kms_sink_;
  GstElement *tee_;
  GstElement *queue_;
  GstElement *convert_;
  GstElement *capsfilter_;

  std::unique_ptr<DRM> p_drm_;

  /*Debug*/mmx::IPadProbeVideoInfoPtr prober_;
};

} // namespace mmx

#endif // OUTPUT_IMPL_KMS_H