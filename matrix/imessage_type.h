#ifndef MATRIX_I_MESSAGE_TYPE_H
#define MATRIX_I_MESSAGE_TYPE_H

#include <cstdint>

namespace mmx {

enum : uint32_t {
  IMSG_TYPE_INVALID = 0,

  IMSG_URIDB_SETPROPERTY,

  IMSG_IMAGE_SETPROPERTY,

  IMSG_TYPE_MAXNUM,
};

} // namespace mmx

#endif // MATRIX_I_MESSAGE_TYPE_H