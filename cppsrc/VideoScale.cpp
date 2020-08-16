//
//  VideoScale.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/6/10.
//  Copyright © 2020 apple. All rights reserved.
//

#include "VideoScale.hpp"

FilterVideoScale::FilterVideoScale():en_ctx(NULL),de_ctx(NULL),ou_fmt(NULL),in_fmt(NULL),inputs(NULL),ouputs(NULL),src_filter_ctx(NULL),sink_filter_ctx(NULL)
,graph(NULL),de_frame(NULL),en_frame(NULL),video_in_index(-1),audio_in_index(-1),video_ou_index(-1),audio_ou_index(-1)
{

}

FilterVideoScale::~FilterVideoScale()
{
    
}

void FilterVideoScale::internalrelease()
{
    if (en_ctx) {
        avcodec_free_context(&en_ctx);
    }
    if (de_ctx) {
        avcodec_free_context(&de_ctx);
    }
    if (in_fmt) {
        avformat_close_input(&in_fmt);
    }
    if (ou_fmt) {
        avformat_free_context(ou_fmt);
        ou_fmt = NULL;
    }
    if (inputs) {
        avfilter_inout_free(&inputs);
    }
    if (ouputs) {
        avfilter_inout_free(&ouputs);
    }
    // 滤镜以及滤镜上下文是通过滤镜管道AVFilterGraph来统一管理和释放资源的
    if (graph) {
        avfilter_graph_free(&graph);
    }
}

void FilterVideoScale::doVideoScale(string srcpath,string dstpath)
{
    dst_width = 840;
    dst_height = 440;
    
    // 打开源文件
    if (avformat_open_input(&in_fmt, srcpath.c_str(), NULL, NULL) < 0) {
        LOGD("open input file fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt, NULL) < 0) {
        LOGD("find input file stream info fail");
        internalrelease();
        return;
    }
    AVCodec *de_codec = NULL;
    // 查找视频流索引，同时返回对应的解码器赋值给de_codec
    if ((video_in_index = av_find_best_stream(in_fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &de_codec, 0)) < 0) {
        LOGD("av_find_best_stream fail");
        internalrelease();
        return;
    }
    // 查找音频流索引，由于不对音频做任何处理，所以第五个参数赋值为NULL;如果输入文件没有音频流那么audio_in_index将为-1
    audio_in_index = av_find_best_stream(in_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    
    // 初始化解码器上下文
    de_ctx = avcodec_alloc_context3(de_codec);
    if (!de_ctx) {
        LOGD("open de_ctx fail");
        internalrelease();
        return;
    }
    
    // 设置解码参数；这里直接从源文件拷贝参数
    avcodec_parameters_to_context(de_ctx, in_fmt->streams[video_in_index]->codecpar);
    
    // 打开解码器上下文
    if (avcodec_open2(de_ctx, de_codec, NULL) < 0) {
        LOGD("avcodec_open2() de_ctx fial");
        internalrelease();
        return;
    }
    
    // 创建目标文件的封装器;用于写入转换后的视频
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstpath.c_str()) < 0) {
        LOGD("avformat_alloc_output_context2() fail");
        internalrelease();
        return;
    }
    
    // 添加音视频流
    AVStream *video_stream = avformat_new_stream(ou_fmt, NULL);
    video_ou_index = video_stream->index;
    AVStream *audio_stream = NULL;
    // 如果源文件中有音频流则添加
    if (audio_in_index != -1) {
        audio_stream = avformat_new_stream(ou_fmt, NULL);
        audio_ou_index = audio_stream->index;
        // 设置音频流参数，这里直接从源拷贝；由于目标文件和源文件格式保持一致，所以不用管code_tag
        avcodec_parameters_copy(audio_stream->codecpar, in_fmt->streams[audio_in_index]->codecpar);
    }
    AVCodec *en_codec = avcodec_find_encoder(de_codec->id);
    en_ctx = avcodec_alloc_context3(en_codec);
    if (!en_ctx) {
        LOGD("encodec avcodec_alloc_context() fail");
        internalrelease();
        return;
    }
    // 设置编码参数
    en_ctx->width = dst_width;
    en_ctx->height = dst_height;
    en_ctx->time_base = in_fmt->streams[video_in_index]->time_base;
    en_ctx->framerate = in_fmt->streams[video_in_index]->r_frame_rate;
    en_ctx->bit_rate = de_ctx->bit_rate * en_ctx->width / dst_width;
    en_ctx->pix_fmt = de_ctx->pix_fmt;
    if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
        en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // 初始化编码器
    if (avcodec_open2(en_ctx, en_codec, NULL) < 0) {
        LOGD("en_ctx fail()");
        internalrelease();
        return;
    }
    // 从编码器拷贝参数
    avcodec_parameters_from_context(video_stream->codecpar, en_ctx);
    
    // 初始化封装器缓冲区
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2() fail");
            internalrelease();
            return;
        }
    }
    
    // 写入文件头
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header fail");
        internalrelease();
        return;
    }
    
    if (!init_vidoo_filter_graph()) {
        internalrelease();
        return;
    }
    
    // 开始从源文件读取音视频数据然后进行再解码编码以及封装
    AVPacket *in_pkt = av_packet_alloc();
    while (av_read_frame(in_fmt, in_pkt) == 0) {
        if (in_pkt->stream_index == video_in_index) {   // 视频数据
            doVideoDecodec(in_pkt);
        } else if(in_pkt->stream_index == audio_in_index){  //音频数据
            // 由于源和目标文件可能使用了不同的时间基，故这里需要进行转换
            AVStream *in_stream = in_fmt->streams[audio_in_index];
            AVStream *ou_stream = ou_fmt->streams[audio_ou_index];
            av_packet_rescale_ts(in_pkt, in_stream->time_base, ou_stream->time_base);
            // 流索引也要对应上
            in_pkt->stream_index = audio_ou_index;
            
            // 写入音频数据
            LOGD("audio pts %d(%s)",in_pkt->pts,av_ts2timestr(in_pkt->pts,&ou_stream->time_base));
            if (av_write_frame(ou_fmt, in_pkt) < 0) {
                LOGD("audio write fail");
                internalrelease();
                av_packet_unref(in_pkt);
                return;
            }
        }
        
        av_packet_unref(in_pkt);
    }
    doVideoDecodec(NULL);
    av_write_trailer(ou_fmt);
    LOGD("结束了");
    
    // 释放资源
    internalrelease();
    
}

bool FilterVideoScale::init_vidoo_filter_graph()
{
    graph = avfilter_graph_alloc();
    if (!graph) {
        LOGD("avfilter_graph_alloc() fail");
        internalrelease();
        return false;
    }
    
    // 1、创建视频输入滤镜；该滤镜作为滤镜管道的第一个滤镜，用于接收要处理的原始视频数据AVFrame
    const AVFilter *src_filter = avfilter_get_by_name("buffer");
    /** 滤镜参数的格式 key1=value1:key2=value2.....
     *  具体的参数参考源码中滤镜AVFilter对应的字段priv_class的options字段，比如视频输入滤镜的定义为AVFilter ff_vsrc_buffer，它的字段为如下：
     *  static const AVOption buffer_options[] = {
         { "width",         NULL,                     OFFSET(w),                AV_OPT_TYPE_INT,      { .i64 = 0 }, 0, INT_MAX, V },
         { "video_size",    NULL,                     OFFSET(w),                AV_OPT_TYPE_IMAGE_SIZE,                .flags = V },
         { "height",        NULL,                     OFFSET(h),                AV_OPT_TYPE_INT,      { .i64 = 0 }, 0, INT_MAX, V },
         { "pix_fmt",       NULL,                     OFFSET(pix_fmt),          AV_OPT_TYPE_PIXEL_FMT, { .i64 = AV_PIX_FMT_NONE }, .min = AV_PIX_FMT_NONE, .max = INT_MAX, .flags = V },
         { "sar",           "sample aspect ratio",    OFFSET(pixel_aspect),     AV_OPT_TYPE_RATIONAL, { .dbl = 0 }, 0, DBL_MAX, V },
         { "pixel_aspect",  "sample aspect ratio",    OFFSET(pixel_aspect),     AV_OPT_TYPE_RATIONAL, { .dbl = 0 }, 0, DBL_MAX, V },
         { "time_base",     NULL,                     OFFSET(time_base),        AV_OPT_TYPE_RATIONAL, { .dbl = 0 }, 0, DBL_MAX, V },
         { "frame_rate",    NULL,                     OFFSET(frame_rate),       AV_OPT_TYPE_RATIONAL, { .dbl = 0 }, 0, DBL_MAX, V },
         { "sws_param",     NULL,                     OFFSET(sws_param),        AV_OPT_TYPE_STRING,                    .flags = V },
         { NULL },
        };
     *  那么只需要设置width,height,pix_fmt,time_base必要字段即可，这些字段的值要和实际的值对应上，切不可乱设置，否则滤镜处理会出错
     */
    char src_args[200] = {0};
    AVStream *video_in_stream = in_fmt->streams[video_in_index];
    snprintf(src_args, sizeof(src_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",de_ctx->width,de_ctx->height,de_ctx->pix_fmt
             ,video_in_stream->time_base.num,video_in_stream->time_base.den);
    // 创建输入滤镜上下文;该上下文用来对滤镜进行参数设置，初始化，连接其它滤镜等等
    int ret = avfilter_graph_create_filter(&src_filter_ctx, src_filter, "in", src_args, NULL, graph);
    if (ret < 0) {
        LOGD("avfilter_graph_create_filter() fail");
        internalrelease();
        return false;
    }
    
    /** 遇到问题：avfilter_graph_parse_ptr()调用时提示"Media type mismatch between the 'Parsed_transpose_1' filter output pad 0 (video) and the 'ou' filter input pad 0 (audio)
    Cannot create the link transpose:0 -> abuffersink:0"
     *  分析原因：视频输出滤镜的名字叫buffersink，错写成了音频输出滤镜的名字abuffersink
     *  解决方案：
     */
    // 2、创建滤镜输出滤镜，该滤镜作为滤镜管道的嘴鸥一个滤镜，用于输出滤镜处理过的视频数据AVFrame
    const AVFilter *sink_filter = avfilter_get_by_name("buffersink");
    if (!sink_filter) {
        LOGD("abuffersink get fail()");
        internalrelease();
        return false;
    }
    // 输出滤镜是不需要参数的
    ret = avfilter_graph_create_filter(&sink_filter_ctx, sink_filter, "ou", NULL, NULL, graph);
    if (ret < 0) {
        LOGD("sink filter ctx create fail()");
        internalrelease();
        return false;
    }
    
    // 3、创建滤镜输入节点，输出节点并对应上输入输出滤镜。这两个节点和通过滤镜描述符创建的滤镜链最终连接成整个完整的滤镜处理链
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *ouputs = avfilter_inout_alloc();
    // inputs节点连接着通过滤镜描述符创建的滤镜链的最后一个滤镜的输出端口，所以它的name设置为"out"
    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink_filter_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;
    
    // ouputs节点连接着通过滤镜描述符创建的滤镜链的第一个滤镜的输入端口，所以它的name设置为"in"
    ouputs->name = av_strdup("in");
    ouputs->filter_ctx = src_filter_ctx;
    ouputs->pad_idx = 0;
    ouputs->next = NULL;
    
    /** 4、定义滤镜描述符字符串，然后通过滤镜描述符以及前面定义的inputs，ouputs节点创建一个完整的滤镜处理链
     *  滤镜描述符的格式：
     *  滤镜名1=滤镜参数名1=滤镜参数值1:滤镜参数名2=滤镜参数值2:.....,滤镜名2=滤镜参数名2=滤镜参数值1:滤镜参数名2=滤镜参数值2:.....,
     */
    // 这里用到了视频压缩滤镜和视频翻转滤镜;主要scale滤镜和transpose滤镜
    /** 遇到问题：avfilter_graph_parse_ptr()调用时提示"No such filter: 'scale' "
     *  分析原因：编译ffmpeg时没有将scale滤镜编译进去
     *  解决方案：通过选项 --enable-filter=scale和--enable-filter=transpose开启。
     */
    /** 遇到问题：编码时提示"Input picture width (840) is greater than stride (448)"
     *  分析原因：由于下面用到了cclock翻转滤镜，意思是视频翻转180度，那么就意味着目标视频的宽高和原视频是互换了，所以对于scale滤镜在进行压缩时就要保持宽高比和原视频反过来
     *  解决方案：scale=代表宽高压缩的比例，如果没有transpose=cclock，应该为snprintf(filter_desc, sizeof(filter_desc), "scale=%d:%d",dst_width,dst_height);由于有了transpose=cclock，
     *  则要将宽高参数调换
     */
    char filter_desc[200] = {0};
    snprintf(filter_desc, sizeof(filter_desc), "scale=%d:%d,transpose=cclock",dst_height,dst_width);
    ret = avfilter_graph_parse_ptr(graph, filter_desc, &inputs, &ouputs, NULL);
    if (ret < 0) {
        LOGD("avfilter_graph_parse_ptr fail");
        internalrelease();
        return false;
    }
    
    // 5、初始化滤镜管道
    if (avfilter_graph_config(graph, NULL) < 0) {
        LOGD("avfilter_graph_config fail");
        internalrelease();
        return false;
    }
    
    return true;
}

void FilterVideoScale::doVideoDecodec(AVPacket *pkt)
{
    avcodec_send_packet(de_ctx, pkt);
    if (!de_frame) {
        de_frame = av_frame_alloc();
    }
    while (true) {
        int ret = avcodec_receive_frame(de_ctx, de_frame);
        if (ret == AVERROR_EOF) {
            doVideoEncode(NULL);
            break;
        } else if(ret < 0) {
            break;
        }
        de_frame->pts = de_frame->best_effort_timestamp;
        
        // 取得了解码数据;送入滤镜管道进行处理
        ret = av_buffersrc_add_frame_flags(src_filter_ctx, de_frame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret < 0) {
            LOGD("av_buffersrc_add_frame_flags fail");
            break;
        }
        // 从滤镜管道获取处理过的视频数据
        if (!en_frame) {
            en_frame = av_frame_alloc();
            en_frame->width = en_ctx->width;
            en_frame->height = en_ctx->height;
            en_frame->format = en_ctx->pix_fmt;
            av_frame_get_buffer(en_frame, 0);
            av_frame_is_writable(en_frame);
        }
        while (true) {
            ret = av_buffersink_get_frame(sink_filter_ctx, en_frame);
            if (ret < 0) {
                break;
            }
            
            // 取得了数据
            doVideoEncode(en_frame);
            /** 必须释放，不然会造成内存泄露
             *  av_buffersink_get_frame()函数就跟avcodec_receive_frame()函数一样，不需提前为AVFrame分配内存，每次调用它内部都会
             *  重新分配新的AVFrame的内存，所以使用完毕后必须手动释放这个AVFrame的内存
             */
            av_frame_unref(en_frame);
        }
    }
}

void FilterVideoScale::doVideoEncode(AVFrame *frame)
{
    avcodec_send_frame(en_ctx, frame);
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        int ret = avcodec_receive_packet(en_ctx, pkt);
        if (ret < 0) {
            av_packet_unref(pkt);
            break;
        }
        
        // 写入之前转换时间基
        av_packet_rescale_ts(pkt, en_ctx->time_base, ou_fmt->streams[video_ou_index]->time_base);
        LOGD("video pts %d(%s)",pkt->pts,av_ts2timestr(pkt->pts, &ou_fmt->streams[video_ou_index]->time_base));
        av_write_frame(ou_fmt, pkt);
        av_packet_unref(pkt);
    }
}
