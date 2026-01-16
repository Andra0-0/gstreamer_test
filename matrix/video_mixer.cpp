#include "video_mixer.h"

#include <sstream>

#include "video_mixer_glvm.h"
#include "video_mixer_rgac.h"
#include "debug.h"

namespace mmx {

VideoMixerPtr VideoMixer::create(VideomixUsage usage)
{
  VideoMixerPtr mixer = nullptr;

  switch (usage) {
    case kVideomixGlvm:
      mixer = std::make_shared<VideoMixerGlvm>();
      break;
    case kVideomixRgac:
      mixer = std::make_shared<VideoMixerRgac>();
      break;
    default:
      ALOGD("VideoMixer create error, unknown usage: %d", usage);
      break;
  }

  return mixer;
}

VideoMixer::VideoMixer()
  : bin_(nullptr)
  , mix_(nullptr)
  , caps_(nullptr)
  , tee_(nullptr)
  , screen_width_(1920)
  , screen_height_(1080)
  , sinkpad_cnt_(0)
{
}

VideoMixer::~VideoMixer()
{
}

VideomixStyle VideoMixer::get_style_from_layout(VideomixLayout layout, gint index)
{
  VideomixStyle style;

  switch (layout) {
    case kLayoutInvalid:
      ALOGD("Invalid paramter layout: %d", layout);
      break;
    case kLayoutSingleScreen:
      style.xpos = 0;
      style.ypos = 0;
      style.width = screen_width_;
      style.height = screen_height_;
      break;
    case kLayoutDualScreen:
      if (index == 0) { // 左半屏
        style.xpos = 0;
        style.ypos = 0;
        style.width = screen_width_ / 2;
        style.height = screen_height_;
      } else { // 右半屏
        style.xpos = screen_width_ / 2;
        style.ypos = 0;
        style.width = screen_width_ / 2;
        style.height = screen_height_;
      }
      break;
    case kLayoutThreeScreen: // 三分屏
      if (index == 0) { // 左侧主画面，占据 2/3 宽度，全高
        style.xpos = 0;
        style.ypos = 180;
        style.width = screen_width_ * 2 / 3;
        style.height = screen_height_ * 2 / 3;
      } else if (index == 1) { // 右上画面，占据右 1/3 宽，1/2 高
        // style.xpos = screen_width_ * 2 / 3;
        // style.ypos = 90;
        // style.width = screen_width_ / 3;
        // style.height = screen_height_ / 2;
        style.xpos = screen_width_ * 2 / 3;
        style.ypos = 180;
        style.width = screen_width_ / 3;
        style.height = screen_height_ / 3;
      } else { // 右下画面，占据右 1/3 宽，1/2 高
        // style.xpos = screen_width_ * 2 / 3;
        // style.ypos = 90 + 360;
        // style.width = screen_width_ / 3;
        // style.height = screen_height_ / 2;
        style.xpos = screen_width_ * 2 / 3;
        style.ypos = 180 + 360;
        style.width = screen_width_ / 3;
        style.height = screen_height_ / 3;
      }
      break;
    default:
      ALOGD("Unknown paramter layout: %d", layout);
      break;
  }

  return style;
}

}