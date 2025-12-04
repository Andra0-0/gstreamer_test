#ifndef MEDIA_MATRIX_H
#define MEDIA_MATRIX_H

#include <mutex>
#include <memory>
#include <vector>
#include <atomic>

// #include "input_intf.h"
#include "media_input_intf.h"
#include "output_intf.h"

#define MEDIA_MATRIX_DOT_DIR "./dot"

namespace mmx {

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

  gint update();

  void exit_pending(bool wait=false) {
    exit_pending_ = true;
    if (wait) {
      // TODO
    }
  }

  static gboolean on_handle_bus_msg_error(GstBus *bus, GstMessage *msg, void *data);
  static gboolean on_handle_bus_msg_eos(GstBus *bus, GstMessage *msg, void *data);

protected:

private:
  static std::mutex inslock_;
  static std::shared_ptr<MediaMatrix> instance_;
  std::atomic<bool> exit_pending_;

  GstElement *pipeline_;
  // GstElement *mix_compositor_;
  // GstElement *sink_;

  // Inputs encapsulated as classes; each input provides a GstBin with a "src" pad
  std::vector<MediaInputIntfPtr> inputs_;

  std::vector<OutputIntfPtr> outputs_;

  GstBus *bus_;
};

} // namespace mmx

#endif // MEDIA_MATRIX_H