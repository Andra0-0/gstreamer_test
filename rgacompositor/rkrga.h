#ifndef __RK_RGA_H__
#define __RK_RGA_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <rga/RgaApi.h>
#include <rga/im2d_type.h>
#include <rga/im2d_common.h>
#include <rga/im2d_buffer.h>
#include <rga/im2d_single.h>
#include <rga/RgaUtils.h> // get_bpp_from_format

#include <vector>

#include "rga_dma_alloc.h"

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

  int                 dma_fd_;
  char*               dma_buf_;
  int                 dma_bufsize_;

public:
  RkrgaContext();
  ~RkrgaContext();

  // RkrgaContext& operator=(const RkrgaContext&other);
  // RkrgaContext& operator=(RkrgaContext &&other);

  // bool wrap(GstBuffer *gstbuf, GstMapFlags flag);
  bool wrap(GstVideoFrame *frame, GstMapFlags flag);
  bool wrap(GstBuffer *buffer, GstVideoInfo *info, GstMapFlags flag);
  bool wrap_dmabuf(GstVideoInfo *info);
  void unwrap();
};

// struct CompositePadInfo;
struct RkrgaContextVector {
  std::vector<RkrgaContext> ctxs;
  std::vector<int> release_fence_fds;
  // std::vector<CompositePadInfo> pads_info;
};

#include <sys/time.h>
struct TestRuntime {
  timeval begin;
  timeval finish;
  static timeval last;
  TestRuntime() {
    gettimeofday(&begin, NULL);
    // fprintf(stderr, "[\033[2m%ld.%06ld\033[0m] enter\n",
    //         begin.tv_sec, begin.tv_usec);
  }
  ~TestRuntime() {
    gettimeofday(&finish, NULL);
    size_t use_us =
        1000000 * (finish.tv_sec - begin.tv_sec) +
        (finish.tv_usec - begin.tv_usec);
    size_t last_us =
        1000000 * (begin.tv_sec - last.tv_sec) +
        (begin.tv_usec - last.tv_usec);
    last = begin;
    fprintf(stderr, "[\033[2m%ld.%06ld\033[0m] use %lu us, last call %lu us\n",
            finish.tv_sec, finish.tv_usec, use_us, last_us);
  }
};

#endif // __RK_RGA_H__