#ifndef OUTPUT_IMPL_KMS_H
#define OUTPUT_IMPL_KMS_H

#include "output_intf.h"
#include "ipad_prober.h"

extern "C" {
#include <xf86drm.h>
#include <xf86drmMode.h>
}

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
    int fd;
    drmModeRes *res;
    drmModePlaneRes *plane_res;
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

  /*Debug*/mmx::IPadProberPtr prober_;
};

#endif // OUTPUT_IMPL_KMS_H