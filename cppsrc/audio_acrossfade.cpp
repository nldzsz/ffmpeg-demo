//
//  audio_acrossfade.cpp
//  demo-mac
//
//  Created by apple on 2020/7/15.
//  Copyright © 2020 apple. All rights reserved.
//

#include "audio_acrossfade.hpp"

AudioAcrossfade::AudioAcrossfade()
{
    infmt1 = NULL;
    infmt2 = NULL;
    oufmt = NULL;
    filterGraph = NULL;
    inputs = NULL;
    ouputs = NULL;
    cur_index = 0;
    de_ctx1 = NULL;
    de_ctx2 = NULL;
    en_ctx = NULL;
    de_frame1 = NULL;
    de_frame2 = NULL;
    source_filter_ctx = NULL;
    sink_filter_ctx = NULL;
}

AudioAcrossfade::~AudioAcrossfade()
{
    
}

void AudioAcrossfade::internalRelease()
{
    if (infmt1) {
        avformat_close_input(&infmt1);
    }
    if (infmt2) {
        avformat_close_input(&infmt2);
    }
    if (oufmt) {
        avformat_free_context(oufmt);
        oufmt = NULL;
    }
    if (filterGraph) {
        avfilter_graph_free(&filterGraph);
    }
    if (inputs) {
        avfilter_inout_free(&inputs);
    }
    if (ouputs) {
        avfilter_inout_free(&ouputs);
    }
    
}

void AudioAcrossfade::doAcrossfade(string apath1, string apath2,string dpath,int duration)
{
    
    if (apath1.length() == 0) {
        LOGD("apath not found");
        return;
    }
    
    if (apath2.length() == 0) {
        LOGD("apath2 not found");
        return;
    }
    
    if (dpath.length() == 0) {
        LOGD("dpath invalid");
        return;
    }
    
    if (!openStream(&infmt1,&de_ctx1,apath1)) {
        LOGD("openStream failed");
        internalRelease();
        return;
    }
    
    if (!openStream(&infmt2,&de_ctx2,apath2)) {
        LOGD("openStream failed");
        internalRelease();
        return;
    }
    
    AVStream *instream = infmt1->streams[0];
    AVStream *instream2 = infmt2->streams[0];
    // 创建编码器
    AVCodec *encodec = avcodec_find_encoder(instream->codecpar->codec_id);
    if (!encodec) {
        LOGD("avcodec_find_encoder failed");
        internalRelease();
        return;
    }
    en_ctx = avcodec_alloc_context3(encodec);
    if (!en_ctx) {
        LOGD("avcodec_alloc_context3 failed");
        internalRelease();
        return;
    }
    // 设置编码参数;这里直接从源文件拷贝
    avcodec_parameters_to_context(en_ctx, instream->codecpar);
    // 打开编码器上下文
    if (avcodec_open2(en_ctx, encodec, NULL) < 0) {
        LOGD("encodec avcodec_open2() failed");
        internalRelease();
        return;
    }
    
    int ret = 0;
    // 创建封装器用于写入拼接的两个音频文件
    if ((ret = avformat_alloc_output_context2(&oufmt, NULL, NULL, dpath.c_str())) < 0) {
        LOGD("avformat_alloc_output_context2() failed");
        internalRelease();
        return;
    }
    // 添加输出流
    AVStream *oustream = avformat_new_stream(oufmt, NULL);
    // 设置输出流参数，这里直接从源文件拷贝。
    if((ret = avcodec_parameters_copy(oustream->codecpar, instream->codecpar))<0){
        LOGD("avcodec_parameters_copy failed");
        internalRelease();
        return;
    }
    // 源文件编码方式一样但是码流格式可能不一样，所以这里将目标的codec_tag设置为0,已解决因为码流格式不一样导致的错误
    oustream->codecpar->codec_tag = 0;
    
    // 打开封装器的输出上下文
    if (!(oufmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&oufmt->pb, dpath.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open() failed");
            internalRelease();
            return;
        }
    }
    
    // 初始化滤镜上下文
    if (!initFilterGraph(duration)) {
        LOGD("filter graph init failed");
        internalRelease();
        return;
    }
    
    // 写入头文件
    if ((ret = avformat_write_header(oufmt, NULL)) < 0) {
        LOGD("avformat_write_header() failed");
        internalRelease();
        return;
    }
    
    AVPacket *inpkt = av_packet_alloc();
    // 处理源文件1
    while (av_read_frame(infmt1, inpkt) >= 0) {
        
        // 需要先解码然后再送入滤镜处理
        LOGD("0:pts %d(%s)",inpkt->pts,av_ts2timestr(inpkt->pts,&instream->time_base));
        doDecodec(inpkt,0);
        
    }
    
    // 刷新缓冲区
    doDecodec(NULL,0);
    
    // 处理源文件2
    while (av_read_frame(infmt2, inpkt) >= 0) {
        
        // 则需要先解码然后再送入滤镜处理
        LOGD("1:pts %d(%s)",inpkt->pts,av_ts2timestr(inpkt->pts,&instream2->time_base));
        doDecodec(inpkt,1);
    }
    
    // 刷新缓冲区
    doDecodec(NULL,1);
    
    LOGD("over finish");
    av_write_trailer(oufmt);
    
    internalRelease();
}

void AudioAcrossfade::doDecodec(AVPacket *pkt,int stream_index)
{
    AVCodecContext *ctx = de_ctx1;
    if (stream_index == 1) {
        ctx = de_ctx2;
    }
    
    int ret = 0;
    if(avcodec_send_packet(ctx, pkt) < 0) {
        LOGD("avcodec_send_packet failed");
        return;
    }
    
    if (!de_frame1) {
        de_frame1 = av_frame_alloc();
    }
    
    while (true) {
        ret = avcodec_receive_frame(ctx, de_frame1);
        if (ret == AVERROR(EAGAIN)) {
            return;
        }
        if (ret < 0) {
            break;
        }
        
        // 解码成功，送入滤镜管道进行处理
        string src_filter_name = "abuffer"+to_string(stream_index);
        string sink_filter_name = "abuffersink";
        AVFilterContext *src_ctx = avfilter_graph_get_filter(filterGraph, src_filter_name.c_str());
        AVFilterContext *sink_ctx = avfilter_graph_get_filter(filterGraph, sink_filter_name.c_str());
        if (!src_ctx || !sink_ctx) {
            return;
        }
        
        ret = av_buffersrc_add_frame_flags(src_ctx, de_frame1,AV_BUFFERSRC_FLAG_KEEP_REF);
        while (1) {
            // 从滤镜管道获取滤镜处理后的AVFrame
            AVFrame *frame = av_frame_alloc();
            ret = av_buffersink_get_frame(sink_ctx,frame);
            if (ret == AVERROR_EOF) {
                // 数据处理完毕了则刷新编码器缓冲区;
                doEncodec(NULL);
            }
            
            if (ret < 0) {
                break;
            }
            
            doEncodec(frame);
            // 处理完毕后释放内存
            av_frame_free(&frame);
        }
    }
    
}

void AudioAcrossfade::doEncodec(AVFrame *frame)
{
    int ret = avcodec_send_frame(en_ctx, frame);
    if (ret < 0) {
        LOGD("avcodec_send_frame failed");
        return;
    }
    while (1) {
        AVPacket *pkt = av_packet_alloc();
        ret = avcodec_receive_packet(en_ctx,pkt);
        if (ret < 0) {
            break;
        }
        
        // 编码成功
        doWrite(pkt);
    }
}

void AudioAcrossfade::doWrite(AVPacket *pkt)
{
    av_packet_rescale_ts(pkt, oufmt->streams[0]->time_base, en_ctx->time_base);
    LOGD("write pkt pts %d(%s)",pkt->pts,av_ts2timestr(pkt->pts, &oufmt->streams[0]->time_base));
    if(av_write_frame(oufmt, pkt) < 0) {
        LOGD("av_write_frame failed");
    }
    av_packet_unref(pkt);
}

bool AudioAcrossfade::openStream(AVFormatContext**infmt,AVCodecContext**de_ctx,string path)
{
    if (infmt == NULL || *infmt != NULL) {
        LOGD("infmt is invalide");
        return false;
    }
    if (de_ctx == NULL || *de_ctx != NULL) {
        LOGD("de_ctx is invalide");
        return false;
    }
    
    // 解封装apath1和apath2用于之后读取数据
    int ret = 0;
    AVFormatContext *fmt = NULL;
    if ((ret = avformat_open_input(&fmt, path.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input apath1 failed");
        return false;
    }
    if ((ret = avformat_find_stream_info(fmt, NULL)) < 0) {
        LOGD("avformat_find_stream_info apath1 failed");
        avformat_close_input(&fmt);
        return false;
    }
    *infmt = fmt;
    
    AVStream *instream = fmt->streams[0];
    // 创建用于源文件1的解码器
    AVCodec *decodec1 = avcodec_find_decoder(instream->codecpar->codec_id);
    if (!decodec1) {
        LOGD("can not found decoder %s",avcodec_get_name(decodec1->id));
        return false;
    }
    AVCodecContext *ctx = avcodec_alloc_context3(decodec1);
    if (!ctx) {
        LOGD("avcodec_alloc_context3() failed");
        avformat_close_input(&fmt);
        return false;
    }
    // 设置解码器参数;这里直接从源文件解封装器拷贝
    if((ret = avcodec_parameters_to_context(ctx, instream->codecpar)) < 0){
        LOGD("decodec avcodec_parameters_to_context() failed");
        avformat_close_input(&fmt);
        return false;
    }
    ctx->pkt_timebase = instream->time_base;
    
    // 打开解码器上下文
    if (avcodec_open2(ctx, decodec1, NULL) < 0) {
        LOGD("decodec avcodec_open2() failed");
        avformat_close_input(&fmt);
        return false;
    }
    
    *de_ctx = ctx;
    
    return true;
}

bool AudioAcrossfade::initFilterGraph(int duration)
{
    // 创建滤镜管道
    filterGraph = avfilter_graph_alloc();
    AVStream *in_stream = infmt1->streams[0];
    // 输入滤镜描述符
    char src_des[200];
    sprintf(src_des, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIX64,
    in_stream->time_base.num,in_stream->time_base.den,in_stream->codecpar->sample_rate,av_get_sample_fmt_name((enum AVSampleFormat)in_stream->codecpar->format),(uint64_t)in_stream->codecpar->channel_layout);
    AVFilterContext *src1_filter_ctx = NULL;
    AVFilterContext *src2_filter_ctx = NULL;
    const AVFilter *src_filter1 = avfilter_get_by_name("abuffer");
    const AVFilter *src_filter2 = avfilter_get_by_name("abuffer");
    int ret = avfilter_graph_create_filter(&src1_filter_ctx, src_filter1, "abuffer0", src_des, NULL, filterGraph);
    if (ret < 0) {
        LOGD("avfilter_graph_create_filter1 fail");
        return false;
    }
    ret = avfilter_graph_create_filter(&src2_filter_ctx, src_filter2, "abuffer1", src_des, NULL, filterGraph);
    if (ret < 0) {
        LOGD("avfilter_graph_create_filter2 fail");
        return false;
    }
    AVFilterContext *sink_filter_ctx = NULL;
    const AVFilter  *sink_filter = avfilter_get_by_name("abuffersink");
    ret = avfilter_graph_create_filter(&sink_filter_ctx, sink_filter, "abuffersink", NULL, NULL, filterGraph);
    if (ret < 0) {
        LOGD("avfilter_graph_create_filter3 fail");
        return false;
    }
    
    /** 用于创建滤镜链的字符串，跟ffmpeg中滤镜命令一样，格式语法如下：
     * 1、滤镜管道由至少一个滤镜链组成(多个滤镜链之间用";"分隔)，每个滤镜链仅且必须代表一条输出链，前一个滤镜链必须通过标签和后一个滤镜链的输入进行关联
     * 2、每一个滤镜链由至少一个滤镜组成(多个滤镜之间用","分隔)，每一个滤镜的语法格式如下
     * [in_link_1]...[in_link_N]filter_name[@id]=arguments[out_link_1]...[out_link_M]
     *
     *  in_link_xx 代表滤镜的输入端口的标签名；out_link_1代表滤镜的输出端口的标签名，如果一个滤镜有两个以上输入或者输出端口，则必须通过这样的标签名来进行区分
     *  @id 代表滤镜上下文的标识名(最终为filter_name@id)
     *  arguments 代表该滤镜的参数,格式如下：
     *      1、每个参数由key=value形式组成，多个参数之间用:分隔，key代表滤镜对应的AVOption数组中的参数名，value为传递给该参数的值，顺序任意
     *      2、参数由value形式组成，多个参数之间用:分隔。此形式下无key值，那么将按照AVOption数组中参数声明的顺序依次传给对应的参数
     *      3、上面1和2的混合形式，不过value必须在前面(而且要满足value形式的规则)，key=value在后面
     *      如果滤镜的参数是数组(ps:比如ff_af_adelay滤镜的delays参数)即它的值为多个值的组合，那么这多个值之间用 | 分割,如：adelay=1500|0|500
     */
    // 这里acrossfade滤镜有两个输入端口，所以必须再它前面定义两个滤镜链且定义acrossfade输入标签，且这两个滤镜链的输出标签要和acrossfade输入标签对应，具体如下：
    char chain[400] = {0};
    sprintf(chain, "aresample=44100[ou1];aresample=44100[ou2];[ou1][ou2]acrossfade@acrossfade=441000:%d:c1=exp:c2=exp",duration);
    ret = avfilter_graph_parse2(filterGraph,chain, &inputs, &ouputs);
    if (ret < 0) {
        LOGD("avfilter_graph_parse_ptr failed");
        return false;
    }
    
    /** 遇到问题：滤镜管道没有正常组织
     *  分析原因：刚开始将abuffer滤镜和abuffersink滤镜也加入前面的滤镜描述符中了，这是错误的，因为滤镜描述符中不能包括abuffer滤镜和abuffersink滤镜。
     *  解决方案：abuffer滤镜和abuffersink要单独和滤镜描述符进行连接
     */
    AVFilterInOut *p = inputs;
    inputs = inputs->next;
    p->next = NULL;
    ret = avfilter_link(src1_filter_ctx, 0, p->filter_ctx, p->pad_idx);
    if (ret < 0) {
        LOGD("avfilter_link 1 failed ");
        return false;
    }
    p = inputs;
    p->next = NULL;
    avfilter_link(src2_filter_ctx, 0, p->filter_ctx, p->pad_idx);
    if (ret < 0) {
        LOGD("avfilter_link 2 failed ");
        return false;
    }
    avfilter_link(ouputs->filter_ctx, 0, sink_filter_ctx, 0);
    if (ret < 0) {
        LOGD("avfilter_link 3 failed ");
        return false;
    }
    
    // 初始化滤镜管道
    ret = avfilter_graph_config(filterGraph, NULL);
    if (ret < 0) {
        LOGD("avfilter_graph_config faied");
        return false;
    }
    
    /** 遇到问题：接收到的AVFrame的nbsamples大于了enc_ctx的frame_size的大小
     *  分析原因：因为acrossfade滤镜处理两个输入流进行淡入淡出算法的处理，所以会缓冲数据，当处理完成后调用av_buffersink_get_frame()时就一次性返回给AVFrame了，这个问题就产生了。
     *  解决方案：设置每次调用av_buffersink_get_frame()的AVFrame的大小
     */
    av_buffersink_set_frame_size(sink_filter_ctx, en_ctx->frame_size);
    
    return true;
}
