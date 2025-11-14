#ifndef _INPUT_DEVICE_H_
#define _INPUT_DEVICE_H_

extern "C" {
#include <gst/gst.h>
#include <glib.h>
}

class InputDevice {
public:
  InputDevice();
  ~InputDevice();

private:
  gint id;
};

#endif // _INPUT_DEVICE_H_