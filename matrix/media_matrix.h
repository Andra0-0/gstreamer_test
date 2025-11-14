#ifndef MEDIA_MATRIX_H
#define MEDIA_MATRIX_H

#include <mutex>
#include <memory>

extern "C" {
#include <gst/gst.h>
}

#define MEDIA_MATRIX_DOT_DIR "./dot"

class MediaMatrix {
public:
  static std::shared_ptr<MediaMatrix> instance() {
    if (!instance_) {
      std::lock_guard<std::mutex> lock(inslock_);
      if (!instance_) {
        instance_ = std::make_shared<MediaMatrix>();
      }
    }
    return instance_;
  }

  MediaMatrix();
  ~MediaMatrix();

  gint init(int argc, char *argv[]);
  gint deinit();

  gint join();

private:
  static std::mutex inslock_;
  static std::shared_ptr<MediaMatrix> instance_;

  GstElement *pipeline_;
  GstElement *source_;
  GstElement *filter_;
  GstElement *sink_;

  GstBus *bus_;
};

#endif // MEDIA_MATRIX_H