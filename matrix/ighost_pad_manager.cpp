#include "ighost_pad_manager.h"

#include <sstream>

#include "debug.h"

namespace mmx {

IGhostPadManager::IGhostPadManager(GstElement *bin, const char *prefix)
  : bin_(bin/*(GstElement*)gst_object_ref(bin)*/)
  , prefix_(prefix)
{
}

IGhostPadManager::~IGhostPadManager()
{
  // if (bin_) {
  //   gst_object_unref(bin_);
  //   bin_ = nullptr;
  // }
}

/**
 * 对目标GstPad创建GhostPad，内部持有，外部不释放
 */
GstPad* IGhostPadManager::add_pad(GstPad *tgt_pad)
{
  if (!tgt_pad || !bin_) {
    ALOGD("IGhostPadManager get_request_pad nullptr");
    return nullptr;
  }
  GstPad *pad = nullptr;

  do {
    auto it = tgtpad_umap_.find(tgt_pad);
    if (it != tgtpad_umap_.end()) { // already exists
      pad = pad_vec_[it->second];
      break;
    }

    pad = inter_add_pad(get_new_pad_id(), tgt_pad);
    ALOG_BREAK_IF(!pad);
    gst_pad_set_active(pad, TRUE);
  } while(0);

  return pad;
}

GstPad* IGhostPadManager::get_pad(GstPad *tgt_pad)
{
  GstPad *pad = nullptr;

  do {
    auto it = tgtpad_umap_.find(tgt_pad);
    if (it == tgtpad_umap_.end()) {
      ALOGD("IGhostPadManager get_pad not found %p-%s",
              tgt_pad, GST_PAD_NAME(tgt_pad));
      break;
    }
    pad = pad_vec_[it->second];
  } while(0);

  return pad;
}

void IGhostPadManager::del_pad(GstPad *tgt_pad)
{
  do {
    auto it = tgtpad_umap_.find(tgt_pad);
    if (it == tgtpad_umap_.end()) {
      ALOGD("IGhostPadManager del_pad not found %p-%s",
              tgt_pad, GST_PAD_NAME(tgt_pad));
      break;
    }
    inter_del_pad(it->second, tgt_pad);
  } while(0);
}

string IGhostPadManager::get_info()
{
  std::ostringstream oss;

  oss << "\n============ IGhostPadManager ============\n"
      << " bin: " << (bin_ ? GST_OBJECT_NAME(bin_) : "NONE")
      << " prefix: " << prefix_ << "\n";

  for (int i = 0; i < pad_vec_.size(); ++i) {
    if (!pad_vec_[i]) continue;
    oss << " id: " << i << " " << GST_PAD_NAME(pad_vec_[i])
        << " linked: " << gst_pad_is_linked(pad_vec_[i])
        << " active: " << gst_pad_is_active(pad_vec_[i]) << "\n";
  }

  oss << "==========================================";

  return oss.str();
}

size_t IGhostPadManager::get_new_pad_id()
{
  size_t id = 0;

  for (; id < pad_vec_.size(); ++id) {
    if (pad_vec_[id] == nullptr) {
      break;
    }
  }

  if (id == pad_vec_.size()) {
    pad_vec_.emplace_back(nullptr);
  }

  return id;
}

GstPad* IGhostPadManager::inter_add_pad(size_t id, GstPad *tgt_pad)
{
  GstPad *pad = nullptr;
  char padname[128] = {0};

  do {
    gst_object_ref(tgt_pad);

    snprintf(padname, sizeof(padname), "%s_%zu", prefix_.c_str(), id);
    pad = gst_ghost_pad_new(padname, tgt_pad);
    ALOG_BREAK_IF(!pad);
    gst_element_add_pad(bin_, pad);

    tgtpad_umap_.emplace(tgt_pad, id);
    padname_umap_.emplace(padname, id);
    pad_vec_[id] = pad;
  } while(0);

  return pad;
}

void IGhostPadManager::inter_del_pad(size_t id, GstPad *tgt_pad)
{
  do {
    GstPad *pad = pad_vec_[id];
    ALOG_BREAK_IF(!pad);

    auto it = tgtpad_umap_.find(tgt_pad);
    if (it != tgtpad_umap_.end()) {
      tgtpad_umap_.erase(it);
    }

    auto it_name = padname_umap_.find(GST_PAD_NAME(pad));
    if (it_name != padname_umap_.end()) {
      padname_umap_.erase(it_name);
    }

    gst_object_unref(tgt_pad);
    gst_object_unref(pad);
    pad_vec_[id] = nullptr;
  } while(0);
}

} // namespace mmx