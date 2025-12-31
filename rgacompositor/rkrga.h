#ifndef __RK_RGA_H__
#define __RK_RGA_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <rga/RgaApi.h>
#include <rga/im2d_type.h>
#include <rga/im2d_common.h>
#include <rga/im2d_buffer.h>
#include <rga/im2d_single.h>

#include <vector>

void
rga_compositor_init_rkrga(void);

void
rga_compositor_deinit_rkrga(void);

struct RkrgaContext {
public:
  rga_buffer_handle_t handle_;
  rga_buffer_t        rgabuf_;
  GstMapInfo          mapinfo_;
  GstMapFlags         mapflag_;
  GstBuffer*          buffer_;

public:
  RkrgaContext();
  ~RkrgaContext();

  // bool wrap(GstBuffer *gstbuf, GstMapFlags flag);
  bool wrap(GstVideoFrame *frame, GstMapFlags flag);
  bool wrap(GstBuffer *buffer, GstVideoInfo *info, GstMapFlags flag);
};

#endif // __RK_RGA_H__