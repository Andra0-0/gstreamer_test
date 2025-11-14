#ifndef _INPUT_DEVICE_H_
#define _INPUT_DEVICE_H_

extern "C" {
#include <gst/gst.h>
#include <glib.h>
}

/**
 * InputDevice interface class
 */
class InputDevice {
public:
  InputDevice();

  virtual ~InputDevice();

private:

};

#endif // _INPUT_DEVICE_H_