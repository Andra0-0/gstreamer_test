#include "media_input_intf.h"

namespace mmx {

MediaInputIntf::MediaInputIntf()
  : id_(-1)
  , state_(kStreamStateInvalid)
  , width_(1920)
  , height_(1080)
{

}

}