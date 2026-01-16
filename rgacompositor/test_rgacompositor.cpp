#include <gst/gst.h>
#include <gst/video/video.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// 结构体用于存储pipeline和相关元素
struct PipelineData {
    GstElement *pipeline;
    GstElement *compositor;
    GstElement *video_sink;
    GstBus *bus;
    
    // 三个视频源
    GstElement *source1;
    GstElement *source2;
    GstElement *source3;
    
    // 对应的转换器和缩放器
    GstElement *convert1, *scale1, *capsf1;
    GstElement *convert2, *scale2, *capsf2;
    GstElement *convert3, *scale3, *capsf3;
    
    GMainLoop *loop;
};

// 设置compositor接收pad的位置和大小
static void configure_compositor_pad(GstElement *compositor, 
                                     const gchar *pad_name,
                                     gint x, gint y, 
                                     gint width, gint height) {
    GstPad *pad = gst_element_get_static_pad(compositor, pad_name);
    if (!pad) {
        printf("无法获取pad: %s\n", pad_name);
        return;
    }
    
    g_object_set(pad, 
                 "xpos", x,
                 "ypos", y,
                 "width", width,
                 "height", height,
                 NULL);
    
    gst_object_unref(pad);
}

// 消息处理函数
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    PipelineData *pdata = static_cast<PipelineData*>(data);
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("播放结束\n");
            g_main_loop_quit(pdata->loop);
            break;
            
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            
            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);
            
            printf("错误: %s\n", error->message);
            g_error_free(error);
            
            g_main_loop_quit(pdata->loop);
            break;
        }
        
        default:
            break;
    }
    
    return TRUE;
}

// 创建pipeline
static PipelineData* create_pipeline() {
    PipelineData *pdata = new PipelineData();

    // 设置GStreamer日志打印
    g_setenv("GST_DEBUG","1,rgacompositor:5,rgacompositor_rkrga:5", TRUE);
    // g_setenv("GST_DEBUG","1", TRUE);
    
    // 初始化GStreamer
    gst_init(NULL, NULL);
    
    // 创建pipeline
    pdata->pipeline = gst_pipeline_new("compositor-pipeline");
    
    // 创建compositor元素
    pdata->compositor = gst_element_factory_make("rgacompositor", "rgacompositor");
    if (!pdata->compositor) {
        printf("Error to create rgacompositor\n");
    }
    
    // 创建视频输出
    pdata->video_sink = gst_element_factory_make("autovideosink", "video-sink");
    
    // 创建三个视频源（这里使用测试源，实际可以替换为file、rtsp等）
    pdata->source1 = gst_element_factory_make("videotestsrc", "source1");
    // pdata->source1 = gst_element_factory_make("uridecodebin", "source1");
    pdata->convert1 = gst_element_factory_make("videoconvert", "convert1");
    pdata->scale1 = gst_element_factory_make("videoscale", "scale1");
    pdata->capsf1 = gst_element_factory_make("capsfilter", "capsf1");
    
    pdata->source2 = gst_element_factory_make("videotestsrc", "source2");
    // pdata->source2 = gst_element_factory_make("uridecodebin", "source2");
    pdata->convert2 = gst_element_factory_make("videoconvert", "convert2");
    pdata->scale2 = gst_element_factory_make("videoscale", "scale2");
    pdata->capsf2 = gst_element_factory_make("capsfilter", "capsf2");
    
    pdata->source3 = gst_element_factory_make("videotestsrc", "source3");
    // pdata->source3 = gst_element_factory_make("uridecodebin", "source3");
    pdata->convert3 = gst_element_factory_make("videoconvert", "convert3");
    pdata->scale3 = gst_element_factory_make("videoscale", "scale3");
    pdata->capsf3 = gst_element_factory_make("capsfilter", "capsf3");
    
    // 检查所有元素是否创建成功
    if (!pdata->pipeline || !pdata->compositor || !pdata->video_sink ||
        !pdata->source1 || !pdata->convert1 || !pdata->scale1 || !pdata->capsf1 ||
        !pdata->source2 || !pdata->convert2 || !pdata->scale2 || !pdata->capsf2 ||
        !pdata->source3 || !pdata->convert3 || !pdata->scale3 || !pdata->capsf3) {
        printf("无法创建所有GStreamer元素\n");
        delete pdata;
        return nullptr;
    }
    
    // 设置视频源属性
    g_object_set(pdata->source1, "pattern", 0, NULL);  // 雪碧
    g_object_set(pdata->source2, "pattern", 1, NULL);  // 白噪声
    g_object_set(pdata->source3, "pattern", 18, NULL); // 小球
    // g_object_set(pdata->source1, "uri", "file:///home/cat/test/test.mp4", NULL);
    // g_object_set(pdata->source2, "uri", "file:///home/cat/test/test.mp4", NULL);
    // g_object_set(pdata->source3, "uri", "file:///home/cat/test/test.mp4", NULL);

    GstCaps *caps;
    caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            NULL);
    if (caps) {
      g_object_set(G_OBJECT(pdata->capsf1), "caps", caps, NULL);
      g_object_set(G_OBJECT(pdata->capsf2), "caps", caps, NULL);
      g_object_set(G_OBJECT(pdata->capsf3), "caps", caps, NULL);
      gst_caps_unref(caps);
    } else {
      printf("Failed to create caps for capsfilter\n");
    }
    
    // 将元素添加到pipeline
    gst_bin_add_many(GST_BIN(pdata->pipeline),
                     pdata->source1, pdata->convert1, pdata->scale1, pdata->capsf1,
                     pdata->source2, pdata->convert2, pdata->scale2, pdata->capsf2,
                     pdata->source3, pdata->convert3, pdata->scale3, pdata->capsf3,
                     pdata->compositor, pdata->video_sink, NULL);
    
    // 链接第一个视频源链
    if (!gst_element_link_many(pdata->source1, pdata->convert1, pdata->scale1, pdata->capsf1, NULL)) {
        printf("无法链接source1\n");
        delete pdata;
        return nullptr;
    }
    
    // 链接第二个视频源链
    if (!gst_element_link_many(pdata->source2, pdata->convert2, pdata->scale2, pdata->capsf2, NULL)) {
        printf("无法链接source2\n");
        delete pdata;
        return nullptr;
    }
    
    // 链接第三个视频源链
    if (!gst_element_link_many(pdata->source3, pdata->convert3, pdata->scale3, pdata->capsf3, NULL)) {
        printf("无法链接source3\n");
        delete pdata;
        return nullptr;
    }
    
    // 将三个视频源链接到compositor
    // 需要获取compositor的sink pad进行链接
    GstPad *sinkpad1 = gst_element_get_request_pad(pdata->compositor, "sink_%u");
    GstPad *srcpad1 = gst_element_get_static_pad(pdata->capsf1, "src");
    
    GstPad *sinkpad2 = gst_element_get_request_pad(pdata->compositor, "sink_%u");
    GstPad *srcpad2 = gst_element_get_static_pad(pdata->capsf2, "src");
    
    GstPad *sinkpad3 = gst_element_get_request_pad(pdata->compositor, "sink_%u");
    GstPad *srcpad3 = gst_element_get_static_pad(pdata->capsf3, "src");
    
    if (!sinkpad1 || !srcpad1 || !sinkpad2 || !srcpad2 || !sinkpad3 || !srcpad3) {
        printf("无法获取pad\n");
        delete pdata;
        return nullptr;
    }
    
    // 链接pad
    if (gst_pad_link(srcpad1, sinkpad1) != GST_PAD_LINK_OK ||
        gst_pad_link(srcpad2, sinkpad2) != GST_PAD_LINK_OK ||
        gst_pad_link(srcpad3, sinkpad3) != GST_PAD_LINK_OK) {
        printf("无法链接pad\n");
        delete pdata;
        return nullptr;
    }
    
    gst_object_unref(srcpad1);
    gst_object_unref(srcpad2);
    gst_object_unref(srcpad3);
    
    // 链接compositor到视频输出
    if (!gst_element_link(pdata->compositor, pdata->video_sink)) {
        printf("无法链接compositor到视频输出\n");
        delete pdata;
        return nullptr;
    }
    
    // 设置消息总线
    pdata->bus = gst_pipeline_get_bus(GST_PIPELINE(pdata->pipeline));
    pdata->loop = g_main_loop_new(NULL, FALSE);
    gst_bus_add_watch(pdata->bus, bus_call, pdata);
    
    return pdata;
}

// 配置compositor布局
static void configure_layout(PipelineData *pdata) {
    // 设置输出视频大小为1280x720
    g_object_set(pdata->compositor, 
                 "background", 1, // 1-black
                 NULL);
    
    // 获取compositor的所有sink pad并配置布局
    // 注意：这里假设pad的名称是sink_0, sink_1, sink_2
    // 在实际应用中，可能需要更动态的方式
    
    // 第一个视频：左上角，640x360
    configure_compositor_pad(pdata->compositor, "sink_0", 0, 0, 640, 360);
    
    // 第二个视频：右上角，640x360
    configure_compositor_pad(pdata->compositor, "sink_1", 640, 0, 640, 360);
    
    // 第三个视频：底部，1280x360
    configure_compositor_pad(pdata->compositor, "sink_2", 0, 360, 1280, 360);
}

PipelineData *pdata = nullptr;

void signal_handler(int signum)
{
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pdata->pipeline),
                GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-playing");
    exit(0);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, signal_handler);

    try {
        g_setenv("GST_DEBUG_DUMP_DOT_DIR", "./", TRUE);

        // 创建并配置pipeline
        pdata = create_pipeline();
        if (!pdata) {
            std::cerr << "创建pipeline失败" << std::endl;
            return -1;
        }
        
        // 配置布局
        configure_layout(pdata);
        
        // 设置pipeline状态为播放
        gst_element_set_state(pdata->pipeline, GST_STATE_PLAYING);
        
        std::cout << "开始播放... 按Ctrl+C停止" << std::endl;
        std::cout << "布局：" << std::endl;
        std::cout << "  视频1: 左上 (640x360)" << std::endl;
        std::cout << "  视频2: 右上 (640x360)" << std::endl;
        std::cout << "  视频3: 底部 (1280x360)" << std::endl;
        
        // 运行主循环
        g_main_loop_run(pdata->loop);
        
        // 停止播放
        gst_element_set_state(pdata->pipeline, GST_STATE_NULL);
        
        // 清理资源
        gst_object_unref(pdata->bus);
        gst_object_unref(pdata->pipeline);
        g_main_loop_unref(pdata->loop);
        
        delete pdata;
        
        std::cout << "程序结束" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        if (pdata) {
            delete pdata;
        }
        return -1;
    }
    
    return 0;
}