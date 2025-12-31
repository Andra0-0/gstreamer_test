#include "rkrga.h"

#include <gst/allocators/gstdmabuf.h>

#include "compositor.h"

GST_DEBUG_CATEGORY_STATIC (gst_rgacompositor_rkrga_debug);
#define GST_CAT_DEFAULT gst_rgacompositor_rkrga_debug

void
rga_compositor_init_rkrga(void)
{
  GST_DEBUG_CATEGORY_INIT (gst_rgacompositor_rkrga_debug,
      "rgacompositor_rkrga", 0,
      "rgacompositor rkrga functions");

  if (imcheckHeader() <= 0) {
    GST_WARNING("Rgacompositor use rga version: "RGA_API_VERSION);
  }
  // c_RkRgaInit();
}

void
rga_compositor_deinit_rkrga(void)
{
  // c_RkRgaDeInit();
}

static RgaSURF_FORMAT
_inter_rgafmt_from_videofmt(GstVideoFormat format) {
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
    default:
      GST_DEBUG("UNKNOWN RK FORMAT: %d", format);
      return RK_FORMAT_UNKNOWN;
  }
}

static bool
_inter_set_rga_rect(rga_rect_t *rect,
                    gint width, gint height,
                    gint hstride, gint vstride, gint format)
{
  gint pixel_stride;

  switch (format) {
    case RK_FORMAT_RGBX_8888:
    case RK_FORMAT_BGRX_8888:
    case RK_FORMAT_RGBA_8888:
    case RK_FORMAT_BGRA_8888:
      pixel_stride = 4;
      break;
    case RK_FORMAT_RGB_888:
    case RK_FORMAT_BGR_888:
      pixel_stride = 3;
      break;
    case RK_FORMAT_RGBA_5551:
    case RK_FORMAT_RGB_565:
      pixel_stride = 2;
      break;
    case RK_FORMAT_YCbCr_420_SP_10B:
    case RK_FORMAT_YCbCr_422_SP:
    case RK_FORMAT_YCrCb_422_SP:
    case RK_FORMAT_YCbCr_422_P:
    case RK_FORMAT_YCrCb_422_P:
    case RK_FORMAT_YCbCr_420_SP:
    case RK_FORMAT_YCrCb_420_SP:
    case RK_FORMAT_YCbCr_420_P:
    case RK_FORMAT_YCrCb_420_P:
      pixel_stride = 1;

      /* RGA requires yuv image rect align to 2 */
      width &= ~1;
      height &= ~1;
      break;
    default:
      return false;
  }

  if (hstride / pixel_stride >= width) hstride /= pixel_stride;

  rga_set_rect(rect, 0, 0, width, height, hstride, vstride, format);

  /*Debug*/GST_DEBUG("rect wh(%dx%d) wstride:%d hstride:%d",
          rect->width, rect->height, rect->wstride, rect->hstride);

  return true;
}

RkrgaContext::RkrgaContext()
  : handle_(0)
  , mapflag_(GST_MAP_FLAG_LAST)
  , buffer_(NULL)
{

}

RkrgaContext::~RkrgaContext()
{
  if (handle_ != 0) {
    releasebuffer_handle(handle_);
    handle_ = 0;
  }
  if (buffer_ && mapflag_ != GST_MAP_FLAG_LAST) {
    gst_buffer_unmap(buffer_, &mapinfo_);
    buffer_ = NULL;
  }
}

bool RkrgaContext::wrap(GstVideoFrame *frame, GstMapFlags flag)
{
  if (handle_ != 0) return true;
  if (!frame) return false;

  GstMemory *mem = NULL;
  gint fd = -1;

  if (gst_buffer_n_memory(frame->buffer) == 1) { // frame use dma-buf memory
    gsize offset, memsize;

    mem = gst_buffer_peek_memory(frame->buffer, 0);
    if (gst_is_dmabuf_memory(mem)) {
      gst_memory_get_sizes(mem, &offset, &memsize);
      if (offset == 0) {
        fd = gst_dmabuf_memory_get_fd(mem);
        handle_ = importbuffer_fd(fd, memsize);
      }
    }
  }
  if (fd <= 0) { // frame use virtual address memory
    mapflag_ = flag;
    buffer_ = frame->buffer;
    gst_buffer_map(frame->buffer, &mapinfo_, mapflag_);
    handle_ = importbuffer_virtualaddr(mapinfo_.data, mapinfo_.size);
  }
  if (handle_ <= 0) { // error to importbuffer
    GST_ERROR("importbuffer error");
    return false;
  }

  gint format = _inter_rgafmt_from_videofmt(GST_VIDEO_FRAME_FORMAT(frame));
  gint width = GST_VIDEO_FRAME_WIDTH(frame);
  gint height = GST_VIDEO_FRAME_HEIGHT(frame);
  gint hstride = GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0);
  gint vstride = GST_VIDEO_FRAME_N_PLANES(frame) == 1
          ? GST_VIDEO_INFO_HEIGHT(&frame->info)
          : GST_VIDEO_INFO_PLANE_OFFSET(&frame->info, 1) / hstride;

  rga_rect_t rect;
  if (!_inter_set_rga_rect(&rect, width, height, hstride, vstride, format)) {
    return false;
  }

  rgabuf_ = wrapbuffer_handle(handle_,
          rect.width, rect.height, rect.format, rect.wstride, rect.hstride);

  return true;
}

bool RkrgaContext::wrap(GstBuffer *buffer, GstVideoInfo *info, GstMapFlags flag)
{
  if (handle_ != 0) return true;
  if (!buffer || !info) return false;

  GstMemory *mem = NULL;
  gint fd = -1;

  if (gst_buffer_n_memory(buffer) == 1) {
    gsize offset, memsize;

    mem = gst_buffer_peek_memory(buffer, 0);
    if (gst_is_dmabuf_memory(mem)) {
      gst_memory_get_sizes(mem, &offset, &memsize);
      if (offset == 0) {
        fd = gst_dmabuf_memory_get_fd(mem);
        handle_ = importbuffer_fd(fd, memsize);
      }
    }
  }
  if (fd <= 0) {
    mapflag_ = flag;
    buffer_ = buffer;
    gst_buffer_map(buffer, &mapinfo_, mapflag_);
    handle_ = importbuffer_virtualaddr(mapinfo_.data, mapinfo_.size);
  }
  if (handle_ <= 0) {
    // FIXME: outbuf not writable at begin of video stream
    if (!gst_buffer_is_writable(buffer)) {
      GST_DEBUG("gst_buffer is not writeable!");
    }
    // return false;
  }

  gint format = _inter_rgafmt_from_videofmt(info->finfo->format);
  gint width = info->width;
  gint height = info->height;
  gint hstride = info->stride[0];
  gint vstride = (info->finfo->n_planes == 1)
          ? info->height
          : info->offset[1] / hstride;

  rga_rect_t rect;
  if (!_inter_set_rga_rect(&rect, width, height, hstride, vstride, format)) {
    return false;
  }

  rgabuf_ = wrapbuffer_handle(handle_,
          rect.width, rect.height, rect.format, rect.wstride, rect.hstride);

  return true;
}