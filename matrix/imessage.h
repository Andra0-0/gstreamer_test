#ifndef MATRIX_I_MESSAGE_H
#define MATRIX_I_MESSAGE_H

#include <string>
#include <memory>
#include <unordered_map>

namespace mmx {

struct Attribute;
class IMessage;
class IMessageThread;

using std::string;
using std::shared_ptr;
using std::unordered_map;
using string_p = string*;
using AttributePtr = shared_ptr<Attribute>;
using IMessagePtr = shared_ptr<IMessage>;

struct Attribute {
public:
  enum AttrType {
    kAttrTypeBool,
    kAttrTypeInt32,
    kAttrTypeInt64,
    kAttrTypeSizet,
    kAttrTypeFloat,
    kAttrTypeString,
  };
  union AttrValue {
    bool vbool_;        // boolean
    int32_t vint32_;    // signed interger
    int64_t vint64_;    // signed long integer
    size_t vsizet_;     // unsigned long integer
    float vfloat_;      // floating point
    string_p vstring_;  // pointer of std::string

    AttrValue() = default;
    AttrValue(bool v) noexcept : vbool_(v) {}
    AttrValue(int32_t v) noexcept : vint32_(v) {}
    AttrValue(int64_t v) noexcept : vint64_(v) {}
    AttrValue(size_t v) noexcept : vsizet_(v) {}
    AttrValue(float v) noexcept : vfloat_(v) {}
    AttrValue(const string &v) : vstring_(new string(v)) {}
    AttrValue(string &&v) : vstring_(new string(std::move(v))) {}
  };

  Attribute(const char *n, AttrValue v, AttrType t):name_(n),value_(v),type_(t) {}
  ~Attribute() { 
    if (type_ == kAttrTypeString && value_.vstring_ != nullptr) {
      delete value_.vstring_;
      value_.vstring_ = nullptr;
    }
  }

public:
  const char *name_;
  AttrValue value_;
  AttrType type_;
};

class IMessage : public std::enable_shared_from_this<IMessage> {
public:
  IMessage();
  ~IMessage();

  void send(uint64_t delay_us=0);
  void send(IMessageThread *const thread, uint64_t delay_us=0);

  pthread_t dst_tid() const { return dst_tid_; }
  uint64_t dst_time() const { return dst_time_; }

  void set_bool(const char *name, bool value);
  void set_int32(const char *name, int32_t value);
  void set_int64(const char *name, int64_t value);
  void set_sizet(const char *name, size_t value);
  void set_float(const char *name, float value);
  void set_string(const char *name, const string &value);

  bool find_bool(const char *name, bool &value);
  bool find_int32(const char *name, int32_t &value);
  bool find_int64(const char *name, int64_t &value);
  bool find_sizet(const char *name, size_t &value);
  bool find_float(const char *name, float &value);
  bool find_string(const char *name, string &value);

protected:
  void emplace_attr(const char *name, AttributePtr &&attr);

private:
  unordered_map<const char*, AttributePtr> umap_attr_;

  pthread_t   dst_tid_;
  uint64_t    dst_time_; // us
};

}

#endif // MATRIX_I_MESSAGE_H