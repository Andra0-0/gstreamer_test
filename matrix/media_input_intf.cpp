#include "media_input_intf.h"

#include "debug.h"

namespace mmx {

MediaInputIntf::MediaInputIntf()
  : id_(-1)
  , state_(kStreamStateInvalid)
  , width_(1920)
  , height_(1080)
{
  do {
    bin_ = gst_bin_new(nullptr);
    ALOG_BREAK_IF(!bin_);

    vpad_mngr_ = std::make_shared<IGhostPadManager>(bin_, "video_src");
  } while(0);
}

}