#include "input_intf.h"

#include "input_impl_v4l2.h"
#include "input_impl_file.h"

const char *name_array[] = {
  "videoinput0",
  "videoinput1",
  "videoinput2",
  "videoinput3",
  "videoinput4",
  "videoinput5",
  "videoinput6",
  "videoinput7",
};

mutex InputIntfManager::lock_;
shared_ptr<InputIntfManager> InputIntfManager::instance_ = nullptr;

InputIntfManager::InputIntfManager() {
  inputs_ = {
    {name_array[0], std::weak_ptr<InputIntf>()},
    {name_array[1], std::weak_ptr<InputIntf>()},
    {name_array[2], std::weak_ptr<InputIntf>()},
    {name_array[3], std::weak_ptr<InputIntf>()},
    {name_array[4], std::weak_ptr<InputIntf>()},
    {name_array[5], std::weak_ptr<InputIntf>()},
    {name_array[6], std::weak_ptr<InputIntf>()},
    {name_array[7], std::weak_ptr<InputIntf>()},
  };
}

InputIntfPtr InputIntfManager::create(InputDeviceType type)
{
  string name;
  InputIntfPtr ret;
  map<string, std::weak_ptr<InputIntf> >::iterator it;

  do {
    std::lock_guard<mutex> lock(lock_);

    for (it = inputs_.begin(); it != inputs_.end(); ++it) {
      if (it->second.expired()) {
        name = it->first;
      }
    }
    if (name.empty()) {
      return nullptr;
    }
    switch (type) {
      case INPUT_TYPE_V4L2:
        ret = std::make_shared<InputImplV4L2>("/dev/video0", 4);
        break;
      case INPUT_TYPE_FILE:
        // todo
        break;
      default:
        return nullptr;
    }
    ret->name_ = name;
    it->second = ret;
  } while(0);

  return ret;
}

void InputIntfManager::destroy(const string &name) {
    std::lock_guard<mutex> lock(lock_);
    auto it = inputs_.find(name);
    if (it != inputs_.end()) {
      it->second = std::weak_ptr<InputIntf>();
    }
  }