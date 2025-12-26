#ifndef __RK_RGA_H__
#define __RK_RGA_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <rga/RgaApi.h>

void
gst_compositor_init_rkrga(void);

void
gst_compositor_deinit_rkrga(void);

RgaSURF_FORMAT
get_rga_format_from_video_format(GstVideoFormat format);

gint
get_pixel_stride_from_rga_format(RgaSURF_FORMAT format);

gboolean
set_rga_info_with_video_info(rga_info_t *info,
                             RgaSURF_FORMAT format,
                             guint xpos,
                             guint ypos,
                             guint width,
                             guint height,
                             guint hstride,
                             guint vstride);

// gboolean
// get_rga_info_from_video_frame(rga_info_t *info,
//                               GstVideoFrame *frame,
//                               GstMapInfo *mapinfo,
//                               GstMapFlags mapflags);

#endif // __RK_RGA_H__