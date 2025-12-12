#include "imessage.h"

#include <sys/time.h>

#include "imessage_thread_manager.h"

namespace mmx {

static inline uint64_t get_current_time_us()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

IMessage::IMessage()
  : dst_tid_(0)
  , dst_time_(0)
{

}

IMessage::~IMessage()
{
  
}

void IMessage::send(uint64_t delay_us)
{
  dst_tid_ = pthread_self();
  dst_time_ = get_current_time_us() + delay_us;
  IMessageThreadManager::instance()->send_message(shared_from_this(), delay_us);
}

void IMessage::send(IMessageThread *const thread, uint64_t delay_us)
{
  dst_tid_ = thread->tid();
  dst_time_ = get_current_time_us() + delay_us;
  IMessageThreadManager::instance()->send_message(shared_from_this(), delay_us);
}

void IMessage::set_bool(const char *name, bool value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeBool);
  emplace_attr(name, std::move(attr));
}

void IMessage::set_int32(const char *name, int32_t value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeInt32);
  emplace_attr(name, std::move(attr));
}

void IMessage::set_int64(const char *name, int64_t value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeInt64);
  emplace_attr(name, std::move(attr));
}

void IMessage::set_sizet(const char *name, size_t value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeSizet);
  emplace_attr(name, std::move(attr));
}

void IMessage::set_float(const char *name, float value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeFloat);
  emplace_attr(name, std::move(attr));
}

void IMessage::set_string(const char *name, const string &value)
{
  AttributePtr attr = std::make_shared<Attribute>(name, value, Attribute::kAttrTypeString);
  emplace_attr(name, std::move(attr));
}

bool IMessage::find_bool(const char *name, bool &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeBool) {
    value = it->second->value_.vbool_;
    return true;
  }
  return false;
}

bool IMessage::find_int32(const char *name, int32_t &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeInt32) {
    value = it->second->value_.vint32_;
    return true;
  }
  return false;
}

bool IMessage::find_int64(const char *name, int64_t &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeInt64) {
    value = it->second->value_.vint64_;
    return true;
  }
  return false;
}

bool IMessage::find_sizet(const char *name, size_t &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeSizet) {
    value = it->second->value_.vsizet_;
    return true;
  }
  return false;
}

bool IMessage::find_float(const char *name, float &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeFloat) {
    value = it->second->value_.vfloat_;
    return true;
  }
  return false;
}

bool IMessage::find_string(const char *name, string &value)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end() && it->second->type_ == Attribute::kAttrTypeString) {
    value = *(it->second->value_.vstring_);
    return true;
  }
  return false;
}

void IMessage::emplace_attr(const char *name, AttributePtr &&attr)
{
  auto it = umap_attr_.find(name);
  if (it != umap_attr_.end()) {
    it->second = attr;
  } else {
    umap_attr_.emplace(name, attr);
  }
}

}