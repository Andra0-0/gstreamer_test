#ifndef _IN_DEVICE_MODULE_H_
#define _IN_DEVICE_MODULE_H_

#include <memory>
#include <mutex>
#include <list>

extern "C" {
#include <gst/gst.h>
}

#include "input_device.h"

using std::list;
using std::shared_ptr;

struct InDeviceConfig {
  gint id;
  gchar name[64];
  gchar dev_path[128];
};

class InDeviceModule {
public:
  explicit InDeviceModule(GstElement *matrix_pipe);

  ~InDeviceModule();

  gint add_indev(const InDeviceConfig &config);

  gint add_indev_many(const list<InDeviceConfig> &configs);

  gint del_indev(gchar *name);

  // gint modify_indev(const InDeviceConfig &config);

  gint find_indev(const gchar *name, shared_ptr<InputDevice> &indev);

private:
  GstElement *pipe_indev_;

  list<shared_ptr<InputDevice> > list_indev_;
};

#endif // _IN_DEVICE_MODULE_H_