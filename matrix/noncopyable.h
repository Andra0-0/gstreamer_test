#ifndef MATRIX_NONCOPYABLE_H
#define MATRIX_NONCOPYABLE_H

namespace mmx {

class NonCopyable {
public:
  NonCopyable() = default;
  virtual ~NonCopyable() = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

}

#endif