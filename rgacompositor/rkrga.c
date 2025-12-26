#include "rkrga.h"

#include <gst/allocators/gstdmabuf.h>

GST_DEBUG_CATEGORY_STATIC (gst_rgacompositor_rkrga_debug);
#define GST_CAT_DEFAULT gst_rgacompositor_rkrga_debug

void
gst_compositor_init_rkrga(void)
{
  GST_DEBUG_CATEGORY_INIT (gst_rgacompositor_rkrga_debug,
      "rgacompositor_rkrga", 0,
      "rgacompositor rkrga functions");

  /*Debug*/GST_DEBUG("Debug");
  c_RkRgaInit();
}

void
gst_compositor_deinit_rkrga(void)
{
  /*Debug*/GST_DEBUG("Debug");
  c_RkRgaDeInit();
}

RgaSURF_FORMAT
get_rga_format_from_video_format(GstVideoFormat format) {
  switch (format) {
    case GST_VIDEO_FORMAT_I420: return RK_FORMAT_YCbCr_420_P;
    case GST_VIDEO_FORMAT_YV12: return RK_FORMAT_YCrCb_420_P;
    case GST_VIDEO_FORMAT_NV12: return RK_FORMAT_YCbCr_420_SP;
    case GST_VIDEO_FORMAT_NV21: return RK_FORMAT_YCrCb_420_SP;
#ifdef HAVE_NV12_10LE40
    case GST_VIDEO_FORMAT_NV12_10LE40: return RK_FORMAT_YCbCr_420_SP_10B;
#endif
    case GST_VIDEO_FORMAT_Y42B: return RK_FORMAT_YCbCr_422_P;
    case GST_VIDEO_FORMAT_NV16: return RK_FORMAT_YCbCr_422_SP;
    case GST_VIDEO_FORMAT_NV61: return RK_FORMAT_YCrCb_422_SP;
    case GST_VIDEO_FORMAT_RGB16: return RK_FORMAT_RGB_565;
    case GST_VIDEO_FORMAT_RGB15: return RK_FORMAT_RGBA_5551;
    case GST_VIDEO_FORMAT_BGR: return RK_FORMAT_BGR_888;
    case GST_VIDEO_FORMAT_RGB: return RK_FORMAT_RGB_888;
    case GST_VIDEO_FORMAT_BGRA: return RK_FORMAT_BGRA_8888;
    case GST_VIDEO_FORMAT_RGBA: return RK_FORMAT_RGBA_8888;
    case GST_VIDEO_FORMAT_BGRx: return RK_FORMAT_BGRX_8888;
    case GST_VIDEO_FORMAT_RGBx: return RK_FORMAT_RGBX_8888;
    default: return RK_FORMAT_UNKNOWN;
  }
}

gint
get_pixel_stride_from_rga_format(RgaSURF_FORMAT format)
{
  switch (format) {
    case RK_FORMAT_RGBX_8888: return 4;
    case RK_FORMAT_BGRX_8888: return 4;
    case RK_FORMAT_RGBA_8888: return 4;
    case RK_FORMAT_BGRA_8888: return 4;
    case RK_FORMAT_RGB_888: return 3;
    case RK_FORMAT_BGR_888: return 3;
    case RK_FORMAT_RGBA_5551: return 2;
    case RK_FORMAT_RGB_565: return 2;
    case RK_FORMAT_YCbCr_420_SP_10B: return 1;
    case RK_FORMAT_YCbCr_422_SP: return 1;
    case RK_FORMAT_YCrCb_422_SP: return 1;
    case RK_FORMAT_YCbCr_422_P: return 1;
    case RK_FORMAT_YCrCb_422_P: return 1;
    case RK_FORMAT_YCbCr_420_SP: return 1;
    case RK_FORMAT_YCrCb_420_SP: return 1;
    case RK_FORMAT_YCbCr_420_P: return 1;
    case RK_FORMAT_YCrCb_420_P: return 1;
    default: return 16;
  }
}

gboolean
set_rga_info_with_video_info(rga_info_t *info,
                             RgaSURF_FORMAT format,
                             guint xpos,
                             guint ypos,
                             guint width,
                             guint height,
                             guint hstride,
                             guint vstride)
{
  if (info->fd < 0 && !info->virAddr) return FALSE;

  gint pixel_stride = _get_pixel_stride_from_rga_format(format);
  if (hstride / pixel_stride >= width) hstride /= pixel_stride;

  info->mmuFlag = 1;
  rga_set_rect(&info->rect, xpos, ypos, width, height, hstride, vstride, format);
  return TRUE;
}

// gboolean
// get_rga_info_from_video_frame(rga_info_t *info,
//                               GstVideoFrame *frame,
//                               GstMapInfo *mapinfo,
//                               GstMapFlags mapflags)
// {
//   /*Debug*/GST_DEBUG("hello");
//   RgaSURF_FORMAT rga_fmt =
//           _get_rga_format_from_video_format(GST_VIDEO_FRAME_FORMAT(frame));

//   guint width = GST_VIDEO_FRAME_WIDTH(frame);
//   guint height = GST_VIDEO_FRAME_HEIGHT(frame);
//   guint hstride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);
//   guint vstride = GST_VIDEO_FRAME_N_PLANES(frame) == 1
//           ? GST_VIDEO_INFO_HEIGHT(&frame->info)
//           : GST_VIDEO_INFO_PLANE_OFFSET(&frame->info, 1) / hstride;

//   if (!_set_rga_info_with_video_info(info,
//           rga_fmt, 0, 0, width, height, hstride, vstride)) {
//     return FALSE;
//   }

//   GstBuffer *inbuf = frame->buffer;
//   if (gst_buffer_n_memory(inbuf) == 1) {
//     GstMemory *mem = gst_buffer_peek_memory(inbuf, 0);
//     gsize offset;

//     if (gst_is_dmabuf_memory(mem)) {
//       gst_memory_get_sizes(mem, &offset, NULL);
//       if (!offset) {
//         info->fd = gst_dmabuf_memory_get_fd(mem);
//       }
//     }
//   }
//   if (info->fd <= 0) {
//     gst_buffer_map(inbuf, mapinfo, mapflags);
//     info->virAddr = mapinfo->data;
//   }
//   return TRUE;
// }