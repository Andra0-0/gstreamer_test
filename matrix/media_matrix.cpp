#include "media_matrix.h"

#include <cstddef>
#include <stddef.h>
#include <vector>
#include <functional>
#include <iostream>

#include "debug.h"
#include "media_define.h"
#include "media_input_impl_hdmi.h"
#include "media_input_module.h"
#include "output_impl_kms.h"

namespace mmx {

std::mutex MediaMatrix::inslock_;
std::shared_ptr<MediaMatrix> MediaMatrix::instance_ = nullptr;

MediaMatrix::MediaMatrix()
  : IMessageThread("MediaMatrix")
  , pipe_playing_(false)
  , pipe_playpend_cnt_(0)
{
  pipeline_ = gst_pipeline_new ("matrix-pipeline");
  bus_ = gst_element_get_bus (pipeline_);

  GstClock *system_clock = gst_system_clock_obtain();
  gst_pipeline_use_clock(GST_PIPELINE(pipeline_), system_clock);
  gst_object_unref(system_clock);

  busmsg_handler_ = thread(&MediaMatrix::handle_bus_msg, this);
}

MediaMatrix::~MediaMatrix()
{

}

gint MediaMatrix::init(int argc, char *argv[])
{
  ALOG_TRACE;
  gint ret = -1;

  do {
    MediaInputModulePtr input_module = MediaInputModule::instance();

    mixer_ = VideoMixer::create(kVideomixRgac);
    ALOG_BREAK_IF(!mixer_);
    mixer_->init();

    // ==============================================
    //           单路输入
    // ==============================================
    // MediaInputIntfPtr input1;
    // MediaInputConfig cfg1;
    // VideomixConfig vmixcfg1;
    // vmixcfg1.index_ = 0;
    // vmixcfg1.layout_ = VideomixLayout::kLayoutSingleScreen;
    // vmixcfg1.style_ = mixer_->get_style_from_layout(vmixcfg1.layout_, vmixcfg1.index_);
    // cfg1.type_ = kMediaInputHdmi;
    // cfg1.uri_ = PATH_INPUT_HDMI_DEV;
    // // cfg1.type_ = kMediaInputUridb;
    // // cfg1.uri_ = "file:///home/cat/test/test.mp4";
    // cfg1.width_ = vmixcfg1.style_.width;
    // cfg1.height_ = vmixcfg1.style_.height;
    // cfg1.srcname_ = "test-input";
    // input1 = input_module->create(cfg1);
    // inputs_.push_back(input1);

    // vmixcfg1.stream_name_ = input1->name();
    // vmixcfg1.src_pad_ = input_module->get_request_pad(input1);
    // vmixcfgs_.push_back(vmixcfg1);

    // ==============================================
    //           三路输入
    // ==============================================
    MediaInputIntfPtr input1;
    MediaInputConfig cfg1;
    VideomixConfig vmixcfg1;
    vmixcfg1.index_ = 0;
    vmixcfg1.layout_ = VideomixLayout::kLayoutThreeScreen;
    vmixcfg1.style_ = mixer_->get_style_from_layout(vmixcfg1.layout_, vmixcfg1.index_);
    cfg1.type_ = kMediaInputHdmi;
    cfg1.uri_ = PATH_INPUT_HDMI_DEV;
    // cfg1.type_ = kMediaInputUridb;
    // cfg1.uri_ = "file:///home/cat/test/test.mp4";
    // cfg1.uri_ = "rtsp://192.168.3.137:8554/mpeg2TransportStreamTest";
    cfg1.width_ = vmixcfg1.style_.width;
    cfg1.height_ = vmixcfg1.style_.height;
    cfg1.srcname_ = "test-input";
    input1 = input_module->create(cfg1);
    inputs_.push_back(input1);

    vmixcfg1.stream_name_ = input1->name();
    vmixcfg1.src_pad_ = input_module->get_request_pad(input1);
    vmixcfgs_.push_back(vmixcfg1);

    MediaInputIntfPtr input2;
    MediaInputConfig cfg2;
    VideomixConfig vmixcfg2;
    vmixcfg2.index_ = 1;
    vmixcfg2.layout_ = VideomixLayout::kLayoutThreeScreen;
    vmixcfg2.style_ = mixer_->get_style_from_layout(vmixcfg2.layout_, vmixcfg2.index_);

    cfg2.type_ = kMediaInputUridb;
    // cfg2.uri_ = "rtsp://192.168.3.137:8554/h264ESVideoTest";
    // cfg2.uri_ = "file:///home/cat/test/test.mp4";
    cfg2.uri_ = "rtsp://192.168.3.137:8554/mpeg2TransportStreamTest";
    cfg2.srcname_ = "test-input2";
    cfg2.width_ = vmixcfg2.style_.width;
    cfg2.height_ = vmixcfg2.style_.height;
    input2 = input_module->create(cfg2);
    inputs_.push_back(input2);

    vmixcfg2.stream_name_ = input2->name();
    vmixcfg2.src_pad_ = input_module->get_request_pad(input2);
    vmixcfgs_.push_back(vmixcfg2);

    MediaInputIntfPtr input3;
    MediaInputConfig cfg3;
    VideomixConfig vmixcfg3;
    vmixcfg3.index_ = 2;
    vmixcfg3.layout_ = VideomixLayout::kLayoutThreeScreen;
    vmixcfg3.style_ = mixer_->get_style_from_layout(vmixcfg3.layout_, vmixcfg3.index_);

    cfg3.type_ = kMediaInputUridb;
    // cfg3.uri_ = "rtsp://192.168.3.137:8554/h264ESVideoTest";
    cfg3.uri_ = "rtsp://192.168.3.137:8554/mpeg2TransportStreamTest";
    // cfg3.uri_ = "file:///home/cat/test/test.mp4";
    cfg3.srcname_ = "test-input3";
    cfg3.width_ = vmixcfg3.style_.width;
    cfg3.height_ = vmixcfg3.style_.height;
    input3 = input_module->create(cfg3);
    inputs_.push_back(input3);

    vmixcfg3.stream_name_ = input3->name();
    vmixcfg3.src_pad_ = input_module->get_request_pad(input3);
    vmixcfgs_.push_back(vmixcfg3);

    // ==============================================
    //           输出
    // ==============================================
    OutputIntfPtr kmsout = std::make_shared<OutputImplKms>(OutputImplKms::HDMI_TX_0);
    outputs_.push_back(kmsout);

    /* Build the pipeline: add input bins, compositor and sink */
    GstElement *input_module_bin = input_module->get_bin();
    GstElement *video_mixer_bin = mixer_->get_bin();
    GstElement *sink_bin = kmsout->bin();

    gst_bin_add_many(GST_BIN(pipeline_), input_module_bin, video_mixer_bin, sink_bin, NULL);

    // GstPad *src_pad = mixer_->get_request_pad("video_src_0");
    // GstPad *sink_pad = kmsout->sink_pad();
    // if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link compositor src pad to sink sink pad");
    //   // gst_object_unref(src_pad);
    //   // gst_object_unref(sink_pad);
    //   break;
    // }
    GstPad *src_pad = mixer_->get_request_pad("video_src_0");
    GstPad *sink_pad = outputs_[0]->sink_pad();
    if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
      ALOGD("Failed to link compositor src pad to sink sink pad");
      break;
    }

    IMessagePtr new_msg = std::make_shared<IMessage>(IMSG_PIPE_SET_PLAYING, this);
    new_msg->send();
    // for (auto it : vmixcfgs_) {
    //     MediaInputIntfPtr indev;
    //     for (auto in : inputs_) {
    //       if (it.stream_name_ == in->name()) {
    //         indev = in;
    //         break;
    //       }
    //     }
    //     it.src_pad_ = MediaInputModule::instance()->get_request_pad(indev);
    //     if (!it.src_pad_) {
    //       ALOGD("Error to get %s src pad", indev->name());
    //       continue;
    //     }
    //     mixer_->connect(it);
    //     if (!it.sink_pad_) {
    //       ALOGD("Videomixer error to get sink pad");
    //       continue;
    //     }
    //     if (gst_pad_link(it.src_pad_, it.sink_pad_) != GST_PAD_LINK_OK) {
    //       ALOGD("Failed to link src pad to sink sink pad");
    //     }
    //   }

    // // 设置管道状态READY
    // ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    // if (ret == GST_STATE_CHANGE_FAILURE) {
    //   ALOGD("Unable to set the pipeline to the ready state.");
    //   gst_object_unref (pipeline_);
    //   return -1;
    // } else {
    //   ALOGD("Set pipeline to ready state success");
    // }

    // 添加输入流到混流
    // VideomixConfig cfg;
    // cfg.index_ = 0;
    // cfg.layout_ = VideomixLayout::kLayoutSingleScreen;
    // cfg.src_pad_ = src_pad1;
    // cfg.style_ = mixer_->get_style_from_layout(cfg.layout_, cfg.index_);
    // mixer_->connect(cfg);
    // GstPadLinkReturn res = gst_pad_link(cfg.src_pad_, cfg.sink_pad_);
    // if (res != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link src pad to sink sink pad, res:%d", res);
    // }
    // VideomixConfig mixcfg;
    // mixcfg.index_ = 0;
    // mixcfg.layout_ = VideomixLayout::kLayoutThreeScreen;
    // mixcfg.src_pad_ = src_pad1;
    // mixcfg.style_ = mixer_->get_style_from_layout(mixcfg.layout_, mixcfg.index_);
    // mixer_->connect(mixcfg);
    // GstPadLinkReturn res = gst_pad_link(mixcfg.src_pad_, mixcfg.sink_pad_);
    // if (res != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link src pad to sink sink pad, res:%d", res);
    // }
    // VideomixConfig mixcfg2;
    // mixcfg2.index_ = 1;
    // mixcfg2.layout_ = VideomixLayout::kLayoutThreeScreen;
    // mixcfg2.src_pad_ = src_pad2;
    // mixcfg2.style_ = mixer_->get_style_from_layout(mixcfg2.layout_, mixcfg2.index_);
    // mixer_->connect(mixcfg2);
    // res = gst_pad_link(mixcfg2.src_pad_, mixcfg2.sink_pad_);
    // if (res != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link src pad to sink sink pad, res:%d", res);
    // }
    // VideomixConfig mixcfg3;
    // mixcfg3.index_ = 2;
    // mixcfg3.layout_ = VideomixLayout::kLayoutThreeScreen;
    // mixcfg3.src_pad_ = src_pad3;
    // mixcfg3.style_ = mixer_->get_style_from_layout(mixcfg3.layout_, mixcfg3.index_);
    // mixer_->connect(mixcfg3);
    // res = gst_pad_link(mixcfg3.src_pad_, mixcfg3.sink_pad_);
    // if (res != GST_PAD_LINK_OK) {
    //   ALOGD("Failed to link src pad to sink sink pad, res:%d", res);
    // }

    // // 设置管道状态PAUSED
    // ret = gst_element_set_state(pipeline_, GST_STATE_PAUSED);
    // if (ret == GST_STATE_CHANGE_FAILURE) {
    //   ALOGD("Unable to set the pipeline to the paused state.");
    //   gst_object_unref (pipeline_);
    //   return -1;
    // } else {
    //   ALOGD("Set pipeline to paused state success");
    // }

    /* Start playing */
    // ret = gst_element_set_state (pipeline_, GST_STATE_PLAYING);
    // if (ret == GST_STATE_CHANGE_FAILURE) {
    //   ALOGD("Unable to set the pipeline to the playing state.");
    //   gst_object_unref (pipeline_);
    //   return -1;
    // } else {
    //   ALOGD("MediaMatrix pipeline set state playing success");
    // }

    /* 在这里生成DOT文件 */
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_),
        GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-init");
    ret = 0;
  } while(0);

  return ret;
}

gint MediaMatrix::deinit()
{
  /* 在结束前再生成一个DOT文件，显示可能的状态变化 */
  GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline_),
      GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-deinit");

  stop_wait();

  if (busmsg_handler_.joinable()) {
    busmsg_handler_.join();
  }

  if (bus_ != nullptr) {
    gst_object_unref(bus_);
    bus_ = nullptr;
  }
  if (pipeline_ != nullptr) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(pipeline_);
    pipeline_ = nullptr;
  }
  inputs_.clear();
  outputs_.clear();
  return 0;
}

void MediaMatrix::handle_bus_msg()
{
  // IMessagePtr imsg = nullptr;
  GstMessage *busmsg = nullptr;
  GstMessageType busmsgtype = (GstMessageType)(
          GST_MESSAGE_EOS | GST_MESSAGE_ERROR |
          GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_CLOCK_LOST);

  while (!is_exit_pending()) {
    busmsg = gst_bus_timed_pop_filtered (bus_, GST_MSECOND * 500, busmsgtype);
    if (is_exit_pending()) {
      break;
    }
    if (busmsg == NULL) {
      continue;
    }

    // GError *err = nullptr;
    // gchar *debug_info = nullptr;

    switch (GST_MESSAGE_TYPE(busmsg)) {
      case GST_MESSAGE_EOS:
        inter_handle_bus_msg_eos(bus_, busmsg, nullptr);
        gst_message_unref (busmsg);
        return;
      case GST_MESSAGE_ERROR:
        inter_handle_bus_msg_error(bus_, busmsg, nullptr);
        gst_message_unref (busmsg);
        break;
      case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(busmsg) == GST_OBJECT(pipeline_)) {
          inter_handle_bus_msg_state_change(bus_, busmsg, this);
        }
        gst_message_unref (busmsg);
        break;
      case GST_MESSAGE_CLOCK_LOST:
        ALOGD("\e[1;31mClock-Lost bus message\e[0m");
        gst_element_set_state(pipeline_, GST_STATE_PAUSED);
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        break;
      default:
        ALOGD("Unexpected message received.");
        gst_message_unref (busmsg);
        return;
    }
  }
  return;
}

gint MediaMatrix::update()
{
  for (auto it : outputs_) {
    it->refresh();
  }

  return 0;
}

gboolean MediaMatrix::inter_handle_bus_msg_error(GstBus *bus, GstMessage *msg, void *data)
{
  ALOG_TRACE;
  gboolean ret = TRUE;
  GError *err = nullptr;
  gchar *debug_info = nullptr;

  do {
    gst_message_parse_error(msg, &err, &debug_info);
    ALOGD("BusError:%s from element:%s", err ? err->message : "(null)", GST_OBJECT_NAME (msg->src));
    ALOGD("BusDebug:%s", debug_info ? debug_info : "none");

    // 先确认是哪个模块
    GstElement *media_input_module = MediaInputModule::instance()->get_bin();
    if (media_input_module && gst_object_has_ancestor(msg->src, GST_OBJECT(media_input_module))) {
      ALOGD("Error comes from MediaInputModule, trying to locate failing input");
      MediaInputModule::instance()->handle_bus_msg_error(bus, msg);
    }

    gst_element_set_state(pipeline_, GST_STATE_PLAYING);

    g_clear_error(&err);
    g_free(debug_info);
  } while(0);

  return ret;
}

gboolean MediaMatrix::inter_handle_bus_msg_eos(GstBus *bus, GstMessage *msg, void *data)
{
  ALOGD("\e[1;31mMediaMatrix End-Of-Stream reached.\e[0m");
  return TRUE;
}

gboolean MediaMatrix::inter_handle_bus_msg_state_change(GstBus *bus, GstMessage *msg, void *data)
{
  IMessagePtr imsg;
  GstState old_state, new_state, pending_state;
  MediaMatrix *const self = static_cast<MediaMatrix*>(data);

  do {
    gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

    switch (new_state) {
      case GST_STATE_PLAYING:
        imsg = std::make_shared<IMessage>(IMSG_PIPE_SET_PLAYING_SUCCESS, self);
        imsg->set_int32("old_state", old_state);
        imsg->set_int32("new_state", new_state);
        imsg->send();
        break;
      default:
        ALOGD("No bus message handle with this state change");
        break;
    }
    ALOGD("Pipeline state changed from %s to %s, pending: %s",
            gst_element_state_get_name(old_state),
            gst_element_state_get_name(new_state),
            gst_element_state_get_name(pending_state));
  } while(0);

  return TRUE;
}

void MediaMatrix::handle_message(const IMessagePtr &msg)
{
  IMessagePtr new_msg;
  GstStateChangeReturn ret;

  do {
    if (msg->what() == IMSG_PIPE_SET_PLAYING) {
      ALOG_BREAK_IF(inputs_.empty() || outputs_.empty());

      // GstPad *src_pad = mixer_->get_request_pad("video_src_0");
      // GstPad *sink_pad = outputs_[0]->sink_pad();
      // if (gst_pad_link(src_pad, sink_pad) != GST_PAD_LINK_OK) {
      //   ALOGD("Failed to link compositor src pad to sink sink pad");
      //   break;
      // }
      for (auto it : vmixcfgs_) {
        MediaInputIntfPtr indev;
        for (auto in : inputs_) {
          if (it.stream_name_ == in->name()) {
            indev = in;
            break;
          }
        }
        it.src_pad_ = MediaInputModule::instance()->get_request_pad(indev);
        if (!it.src_pad_) {
          ALOGD("Error to get %s src pad", indev->name());
          continue;
        }
        mixer_->connect(it);
        if (!it.sink_pad_) {
          ALOGD("Videomixer error to get sink pad");
          continue;
        }
        if (gst_pad_link(it.src_pad_, it.sink_pad_) != GST_PAD_LINK_OK) {
          ALOGD("Failed to link src pad to sink sink pad");
        }
      }
      ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
      ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

      new_msg = std::make_shared<IMessage>(IMSG_PIPE_SET_PLAYING_PENDING, this);
      new_msg->send(100000); // 100ms

    } else if (msg->what() == IMSG_PIPE_SET_PLAYING_PENDING) {
      // ALOGD("MediaMatrix pipeline set playing pending check");
      if (!pipe_playing_) {
        pipe_playpend_cnt_++;
        if (pipe_playpend_cnt_ > 100) { // 10s timeout
          new_msg = std::make_shared<IMessage>(IMSG_PIPE_SET_PLAYING_TIMEOUT, this);
          new_msg->send();
          pipe_playpend_cnt_ = 0;
        } else {
          new_msg = std::make_shared<IMessage>(IMSG_PIPE_SET_PLAYING_PENDING, this);
          new_msg->send(100000); // 100ms pending check
        }
      } else { // pipeline set playing success
        break;
      }

    } else if (msg->what() == IMSG_PIPE_SET_PLAYING_TIMEOUT) {
      ALOGD("MediaMatrix pipeline set playing timeout, reset state");
      // ret = gst_element_set_state(pipeline_, GST_STATE_NULL);
      // ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

      new_msg = std::make_shared<IMessage>(IMSG_PIPE_RESET_PLAYING, this);
      new_msg->send();

    } else if (msg->what() == IMSG_PIPE_SET_PLAYING_SUCCESS) {
      ALOGD("MediaMatrix pipeline set playing success");
      pipe_playing_ = true;

    } else if (msg->what() == IMSG_PIPE_RESET_PLAYING) {
      if (pipe_playing_) {
        ret = gst_element_set_state(pipeline_, GST_STATE_PAUSED);
        ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);

        ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
        ALOG_BREAK_IF(ret == GST_STATE_CHANGE_FAILURE);
      }
    }
  } while(0);
}

} // namespace mmx