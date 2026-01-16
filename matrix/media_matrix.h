#ifndef MEDIA_MATRIX_H
#define MEDIA_MATRIX_H

#include <mutex>
#include <memory>
#include <vector>
#include <atomic>
#include <thread>

// #include "input_intf.h"
#include "media_input_intf.h"
#include "output_intf.h"
#include "video_mixer.h"
#include "ipad_prober.h"
#include "imessage_thread.h"

namespace mmx {

using std::thread;
using std::vector;

class MediaMatrix : public IMessageThread {
public:
  static MediaMatrix* instance() {
    if (!instance_) {
      std::lock_guard<std::mutex> lock(inslock_);
      if (!instance_) {
        instance_ = std::make_shared<MediaMatrix>();
      }
    }
    return instance_.get();
  }

  MediaMatrix();
  virtual ~MediaMatrix();

  gint init(int argc, char *argv[]);
  gint deinit();

  gint update();

  void handle_bus_msg();

protected:
  virtual void handle_message(const IMessagePtr &msg) override;

  gboolean inter_handle_bus_msg_eos(GstBus *bus, GstMessage *msg, void *data);
  gboolean inter_handle_bus_msg_error(GstBus *bus, GstMessage *msg, void *data);
  gboolean inter_handle_bus_msg_state_change(GstBus *bus, GstMessage *msg, void *data);

private:
  static std::mutex inslock_;
  static std::shared_ptr<MediaMatrix> instance_;

  GstElement *pipeline_;
  // GstElement *mix_compositor_;
  // GstElement *sink_;

  // Inputs encapsulated as classes; each input provides a GstBin with a "src" pad
  std::vector<MediaInputIntfPtr> inputs_;
  VideoMixerPtr mixer_;
  vector<VideomixConfig> vmixcfgs_;
  std::vector<OutputIntfPtr> outputs_;

  bool pipe_playing_;
  gint pipe_playpend_cnt_;

  GstBus* bus_;
  thread  busmsg_handler_;
  /*Debug*/IPadProberPtr prober_;
};

} // namespace mmx

#endif // MEDIA_MATRIX_H