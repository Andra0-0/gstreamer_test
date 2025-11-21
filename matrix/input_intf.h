#ifndef _INPUT_INTF_H_
#define _INPUT_INTF_H_

#include <string>
#include <memory>
#include <mutex>
#include <map>

extern "C" {
#include <gst/gst.h>
#include <glib.h>
}

class InputIntf;
class InputIntfManager;

using std::string;
using std::shared_ptr;
using std::mutex;
using std::map;
using InputIntfPtr = shared_ptr<InputIntf>;
using InputIntfManagerPtr = shared_ptr<InputIntfManager>;

enum InputDeviceType {
  INPUT_TYPE_V4L2,
  INPUT_TYPE_FILE,
};

class InputIntfManager {
public:
  static InputIntfManagerPtr instance() {
    if (!instance_) {
      std::lock_guard<std::mutex> lock(lock_);
      if (!instance_) {
        instance_ = shared_ptr<InputIntfManager>(new InputIntfManager);
      }
    }
    return instance_;
  }

  InputIntfPtr create(InputDeviceType type);

  void destroy(const string &name);

private:
  InputIntfManager(const InputIntfManager&) = delete;
  InputIntfManager& operator=(const InputIntfManager&) = delete;

  InputIntfManager();

private:
  static mutex lock_;
  static shared_ptr<InputIntfManager> instance_;
  map<string, std::weak_ptr<InputIntf> > inputs_;
};

/**
 * Input device interface class
 */
class InputIntf {
public:
  // InputIntf();

  virtual ~InputIntf() {
    InputIntfManager::instance()->destroy(name_);
  }

  virtual const char *name() {
    return name_.c_str();
  }

  // Return GstBin that should be added to the pipeline
  virtual GstElement* bin() = 0;

  // Return the source pad (video) from the bin to link to compositor
  virtual GstPad* src_pad() = 0;

  friend class InputIntfManager;
protected:
  string name_;
};

#endif // _INPUT_INTF_H_