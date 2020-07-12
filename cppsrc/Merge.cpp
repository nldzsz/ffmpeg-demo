//
//  Merge.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/20.
//  Copyright © 2020 apple. All rights reserved.
//

#include "Merge.hpp"
Merge::Merge()
{
    in_fmt1 = NULL;
    in_fmt2 = NULL;
    ou_fmt  = NULL;
    video1_in_index = -1;
    audio1_in_index = -1;
    video2_in_index = -1;
    audio2_in_index = -1;
    video_ou_index = -1;
    audio_ou_index = -1;
    next_video_pts = 0;
    next_audio_pts = 0;
    
    curIndex = {0,-1,-1};
    preIndex = {0,-1,-1};
    ou_fmt   = NULL;
    video_de_frame = NULL;
    audio_de_frame = NULL;
    video_en_frame = NULL;
    audio_en_frame = NULL;
    swsctx = NULL;
    swrctx = NULL;
    srcPaths = vector<string>();
    in_fmts  = vector<AVFormatContext*>();
    de_ctxs = vector<DeCtx>();
    in_indexes = vector<MediaIndex>();
    video_en_ctx = NULL;
    audio_en_ctx = NULL;
    audio_buffer = NULL;
}

Merge::~Merge()
{
    
}

void Merge::MergeTwo(string srcPath1,string srcPath2,string dstPath)
{
    /** 媒体格式类型：
     *  媒体格式分为流式和非流式。主要区别在于两种文件格式如何嵌入元信息，非流式的元信息通常存储在文件中开头，有时在结尾；而流式的元信息跟
     *  具体音视频数据同步存放的。所以多个流式文件简单串联在一起形成新的文件也能正常播放。而多个非流式文件的合并则可能要重新编解码才可以
     *  如下mpg格式就是流式格式，通过直接依次取出每个文件的AVPacket，然后依次调用av_write_frame()即可实现文件合并
     *  如下mp4格式就是非流式格式，如果采用上面的流程合并则要求各个文件具有相同的编码方式，分辨率，像素格式等等才可以，否则就会失败。因为非流式格式的元信息只能描述一种类型的音
     *  视频数据
    */
    
    // 打开要合并的源文件1
    int ret = 0;
    /** 遇到问题：解封装mpg格式视频编码参数始终不正确，提示"[mp3float @ 0x104808800] Header missing"
     *  分析原因：mpg格式对应的demuxer为ff_mpegps_demuxer，它没有.extensions字段(ff_mov_demuxer格式就有)，所以最终它会靠read_probe对应的
     *  方法去分析格式,最终会调用到av_probe_input_format3()中去，该方法又会重新用每个解封装器进行解析为ff_mpegvideo_demuxer，如果没有将该接封装器封装进去则就会出问题
     *  解决方案：要想解封装mpg格式的视频编码参数，必须要同时编译ff_mpegps_demuxer和ff_mpegvideo_demuxer及ff_mpegvideo_parser,ff_mpeg1video_decoder,ff_mp2float_decoder
     */
    if ((ret = avformat_open_input(&in_fmt1,srcPath1.c_str(),NULL,NULL)) < 0) {
        LOGD("avformat_open_input in_fmt1 fail %s",av_err2str(ret));
        return;
    }
    if (avformat_find_stream_info(in_fmt1, NULL) < 0) {
        LOGD("avformat_find_stream_info infmt1 fail");
        releasesources();
        return;
    }
    // 将源文件1音视频索引信息找出来
    for (int i=0; i<in_fmt1->nb_streams; i++) {
        AVStream *stream = in_fmt1->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video1_in_index == -1) {
            video1_in_index = i;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio1_in_index == -1) {
            audio1_in_index = i;
        }
    }
    
    // 打开要合并的源文件2
    if (avformat_open_input(&in_fmt2,srcPath2.c_str(),NULL,NULL) < 0) {
        LOGD("avformat_open_input in_fmt2 fail");
        releasesources();
        return;
    }
    if (avformat_find_stream_info(in_fmt2, NULL) < 0) {
        LOGD("avformat_find_stream_info in_fmt2 fail");
        releasesources();
        return;
    }
    // 将源文件2音视频索引信息找出来
    for (int i=0; i<in_fmt2->nb_streams; i++) {
        AVStream *stream = in_fmt2->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video2_in_index == -1) {
            video2_in_index = i;
        }
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio2_in_index == -1) {
            audio2_in_index = i;
        }
    }
    
    // 打开最终要封装的文件
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstPath.c_str()) < 0) {
        LOGD("avformat_alloc_out_context2() fail");
        releasesources();
        return;
    }
    
    // 为封装的文件添加视频流信息;由于假设两个文件的视频流具有相同的编码方式，这里就是简单的流拷贝
    for (int i=0;i<1;i++) {
        if (video1_in_index != -1) {
            AVStream *stream = avformat_new_stream(ou_fmt,NULL);
            video_ou_index = stream->index;
            
            // 由于是流拷贝方式
            AVStream *srcstream = in_fmt1->streams[video1_in_index];
            if (avcodec_parameters_copy(stream->codecpar, srcstream->codecpar) < 0) {
                LOGD("avcodec_parameters_copy fail");
                releasesources();
                return;
            }
            
            // codec_id和code_tag共同决定了一种编码方式在容器里面的码流格式，所以如果源文件与目的文件的码流格式不一致，那么就需要将目的文件
            // 的code_tag 设置为0，当调用avformat_write_header()函数后会自动将两者保持一致
            unsigned int src_tag = srcstream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmt->oformat->codec_tag, src_tag) != stream->codecpar->codec_tag) {
                stream->codecpar->codec_tag = 0;
            }
            break;
        }
        if (video2_in_index != -1) { // 只要任何一个文件有视频流都创建视频流
            AVStream *stream = avformat_new_stream(ou_fmt,NULL);
            video_ou_index = stream->index;
            
            // 由于是流拷贝方式
            AVStream *srcstream = in_fmt2->streams[video2_in_index];
            if (avcodec_parameters_copy(stream->codecpar,srcstream->codecpar) < 0) {
                LOGD("avcodec_parameters_copy 2 fail");
                releasesources();
                return;;
            }
            
            unsigned int src_tag = srcstream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmt->oformat->codec_tag,src_tag) != stream->codecpar->codec_tag) {
                stream->codecpar->codec_tag = 0;
            }
        }
    }
    
    // 为封装的文件添加流信息;由于假设两个文件的视频流具有相同的编码方式，这里就是简单的流拷贝
    for (int i=0;i<1;i++) {
        if (audio1_in_index != -1) {
            AVStream *dststream = avformat_new_stream(ou_fmt,NULL);
            AVStream *srcstream = in_fmt1->streams[audio1_in_index];
            if (avcodec_parameters_copy(dststream->codecpar,srcstream->codecpar) < 0) {
                LOGD("avcodec_parameters_copy1 fail");
                releasesources();
                return;
            }
            audio_ou_index = dststream->index;
            break;
        }
        
        if (audio2_in_index != -1) {
            AVStream *dststream = avformat_new_stream(ou_fmt, NULL);
            AVStream *srcstream = in_fmt2->streams[audio1_in_index];
            if (avcodec_parameters_copy(dststream->codecpar,srcstream->codecpar) < 0) {
                LOGD("avcodec_parameters_copy 1 fial");
                releasesources();
                return;
            }
            audio_ou_index = dststream->index;
        }
    }
    
    // 打开输出上下文
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ou_fmt->pb,dstPath.c_str(),AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open fail");
            releasesources();
            return;
        }
    }
    
    /** 遇到问题：写入mpg容器时提示"mpeg1video files have exactly one stream"
     *  分析原因：编译mpg的封装器错了，之前写的--enable-muxer=mpeg1video实际上应该是--enable-muxer=mpeg1system
     *  解决方案：编译mpg的封装器换成--enable-muxer=mpeg1system
     */
    // 写入文件头
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header fail");
        releasesources();
        return;
    }
    
    // 进行流拷贝；源文件1
    AVPacket *in_pkt1 = av_packet_alloc();
    while(av_read_frame(in_fmt1,in_pkt1) >=0) {
        
        // 视频流
        if (in_pkt1->stream_index == video1_in_index) {
            
            AVStream *srcstream = in_fmt1->streams[in_pkt1->stream_index];
            AVStream *dststream = ou_fmt->streams[video_ou_index];
            // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
            next_video_pts = max(in_pkt1->pts + in_pkt1->duration,next_video_pts);
            av_packet_rescale_ts(in_pkt1, srcstream->time_base, dststream->time_base);
            // 更正目标流的索引
            in_pkt1->stream_index = video_ou_index;
            
            LOGD("pkt1 video %d(%6s) du %d(%s)",in_pkt1->pts,av_ts2timestr(in_pkt1->pts,&dststream->time_base),in_pkt1->duration,av_ts2timestr(in_pkt1->duration,&dststream->time_base));
        }
        
        // 音频流
        if (in_pkt1->stream_index == audio1_in_index) {
            AVStream *srcstream = in_fmt1->streams[in_pkt1->stream_index];
            AVStream *dststream = ou_fmt->streams[audio_ou_index];
            next_audio_pts = max(in_pkt1->pts + in_pkt1->duration,next_audio_pts);
            // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
            av_packet_rescale_ts(in_pkt1, srcstream->time_base, dststream->time_base);
            // 更正目标流的索引
            in_pkt1->stream_index = audio_ou_index;
            LOGD("pkt1 audio %d(%6s) du%d(%s)",in_pkt1->pts,av_ts2timestr(in_pkt1->pts,&dststream->time_base),in_pkt1->duration,av_ts2timestr(in_pkt1->duration,&dststream->time_base));
        }
        
        // 向封装器中写入数据
        if((ret = av_write_frame(ou_fmt, in_pkt1)) < 0) {
            LOGD("av_write_frame fail1 %s",av_err2str(ret));
            releasesources();
            return;;
        }
    }
    
    // 进行流拷贝；源文件1
    while(true) {
        AVPacket *in_pkt2 = av_packet_alloc();
        if(av_read_frame(in_fmt2,in_pkt2) <0 ) break;
        /** 遇到问题：写入数据是提示"[mp4 @ 0x10100ba00] Application provided invalid, non monotonically increasing dts to muxer in stream 1: 4046848 >= 0"
         *  分析原因：因为是两个源文件进行合并，对于每一个源文件来说，它的第一个AVPacket的pts都是0开始的
         *  解决方案：所以第二个源文件的pts,dts,duration就应该加上前面源文件的duration最大值
         */
        // 视频流
        if (in_pkt2->stream_index == video2_in_index) {
            
            AVStream *srcstream = in_fmt2->streams[in_pkt2->stream_index];
            AVStream *dststream = ou_fmt->streams[video_ou_index];
            if (next_video_pts > 0) {
                AVStream *srcstream2 = in_fmt1->streams[video1_in_index];
                in_pkt2->pts = av_rescale_q(in_pkt2->pts, srcstream->time_base, srcstream2->time_base) + next_video_pts;
                in_pkt2->dts = av_rescale_q(in_pkt2->dts, srcstream->time_base, srcstream2->time_base) + next_video_pts;
                in_pkt2->duration = av_rescale_q(in_pkt2->duration, srcstream->time_base, srcstream2->time_base);
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream2->time_base, dststream->time_base);
            } else {
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream->time_base, dststream->time_base);
            }
            LOGD("pkt2 video %lld(%s)",in_pkt2->pts,av_ts2timestr(in_pkt2->pts,&dststream->time_base));
            // 更正目标流的索引
            in_pkt2->stream_index = video_ou_index;
        }
        
        // 音频流
        if (in_pkt2->stream_index == audio2_in_index) {
            if (in_pkt2->pts == AV_NOPTS_VALUE) {
                in_pkt2->pts = in_pkt2->dts;
            }
            if (in_pkt2->dts == AV_NOPTS_VALUE) {
                in_pkt2->dts = in_pkt2->pts;
            }
            AVStream *srcstream = in_fmt2->streams[in_pkt2->stream_index];
            AVStream *dststream = ou_fmt->streams[audio_ou_index];
            if (next_audio_pts > 0) {
                AVStream *srcstream2 = in_fmt1->streams[audio1_in_index];
                in_pkt2->pts = av_rescale_q(in_pkt2->pts, srcstream->time_base, srcstream2->time_base) + next_audio_pts;
                in_pkt2->dts = av_rescale_q(in_pkt2->dts, srcstream->time_base, srcstream2->time_base) + next_audio_pts;
                in_pkt2->duration = av_rescale_q(in_pkt2->duration, srcstream->time_base, srcstream2->time_base);
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream2->time_base, dststream->time_base);
            } else {
                // 由于源文件和目的文件的时间基可能不一样，所以这里要将时间戳进行转换
                av_packet_rescale_ts(in_pkt2, srcstream->time_base, dststream->time_base);
            }
            LOGD("pkt2 audio %d(%s)",in_pkt2->pts,av_ts2timestr(in_pkt2->pts,&dststream->time_base));
            // 更正目标流的索引
            in_pkt2->stream_index = audio_ou_index;
        }
        
        // 向封装器中写入数据
        if(av_write_frame(ou_fmt, in_pkt2) < 0) {
            LOGD("av_write_frame 2 fail");
            releasesources();
            return;
        }
        
        av_packet_unref(in_pkt2);
    }
    
    // 写入文件尾部信息
    av_write_trailer(ou_fmt);
    LOGD("文件写入完毕...");
    // 释放资源
    releasesources();
}

// 合并非流式容器
void Merge::MergeFiles(string srcPath1,string srcPath2,string dPath1)
{
    dstpath  = dPath1;
    
    srcPaths.push_back(srcPath1);
    srcPaths.push_back(srcPath2);
    
    if (!openInputFile()) {
        return;
    }
    
    if (!addStream()) {
        return;
    }
    
    // 创建AVFRame
    updateAVFrame();
    
    // 开始解码和重新编码
    for (int i=0; i<in_fmts.size();i++) {
        AVFormatContext *in_fmt = in_fmts[i];
        curIndex = in_indexes.at(i);
        // 读取数据
        while (true) {
            AVPacket *pkt = av_packet_alloc();
            if (av_read_frame(in_fmt, pkt) < 0) {
                LOGD("没有数据了 %d",i);
                av_packet_unref(pkt);
                break;
            }
            
            // 解码
            doDecode(pkt,pkt->stream_index == curIndex.video_index);
            // 使用完毕后释放AVPacket
            av_packet_unref(pkt);
        }
        
        // 刷新解码缓冲区;同时刷新两个
        doDecode(NULL,true);
        doDecode(NULL,false);
        
        LOGD("finish file %d",i);
    }
    
    // 写入文件尾部
    if(av_write_trailer(ou_fmt) < 0) {
        LOGD("av_write_trailer fail");
    }
    LOGD("结束写入");
    
    // 是否资源
    releasesources();
}

bool Merge::openInputFile()
{
    MediaIndexCmd(wantIndex,0,-1,-1,INT_MAX,INT_MAX,0,AV_PIX_FMT_NONE,AV_SAMPLE_FMT_NONE,0,AV_CODEC_ID_NONE,AV_CODEC_ID_NONE,0,0,0);
    
    int ret = 0;
    for (int i=0; i<srcPaths.size(); i++) {
        string srcpath = srcPaths[i];
        AVFormatContext *in_fmt = NULL;
        if ((ret = avformat_open_input(&in_fmt, srcpath.c_str(), NULL, NULL)) < 0) {
            LOGD("%d avformat_open_input fail %d",i,ret);
            releasesources();
            return false;
        }
        
        if ((ret = avformat_find_stream_info(in_fmt, NULL)) < 0) {
            LOGD("%d avformat_find_stream_info fail %d",i,ret);
            releasesources();
            return false;
        }
        
        in_fmts.push_back(in_fmt);
        
        // 遍历出源文件的音视频信息
        MediaIndex index;
        MediaIndexCmd(index,i,-1,-1,INT_MAX,INT_MAX,0,AV_PIX_FMT_NONE,AV_SAMPLE_FMT_NONE,0,AV_CODEC_ID_NONE,AV_CODEC_ID_NONE,0,0,0);
        DeCtx dectx = {NULL,NULL};
        for (int j=0; j<in_fmt->nb_streams; j++) {
            
            AVStream *stream = in_fmt->streams[j];
            // 创建解码器
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            AVCodecContext *ctx = avcodec_alloc_context3(codec);
            if (ctx == NULL) {
                LOGD("avcodec_alloc_context3 fail");
                releasesources();
                return false;
            }
            // 设置解码参数
            if((ret = avcodec_parameters_to_context(ctx, stream->codecpar)) < 0) {
                LOGD("avcodec_parameters_to_context");
                releasesources();
                return false;
            }
            // 初始化解码器
            if ((ret = avcodec_open2(ctx, codec, NULL)) < 0) {
                LOGD("avcodec_open2 fail %d",ret);
                releasesources();
                return false;
            }
            
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {   // 视频
                index.video_index = j;
                index.width = stream->codecpar->width;
                index.height = stream->codecpar->height;
                index.video_codecId = stream->codecpar->codec_id;
                index.pix_fmt = (enum AVPixelFormat)stream->codecpar->format;
                index.video_bit_rate = stream->codecpar->bit_rate;
                index.fps = stream->r_frame_rate.num;
                
                // 输出文件的视频编码参数
                if (stream->codecpar->width < wantIndex.width) {  // 取出最小值
                    wantIndex.width = stream->codecpar->width;
                    wantIndex.height = stream->codecpar->height;
                    wantIndex.video_codecId = stream->codecpar->codec_id;
                    wantIndex.pix_fmt = (enum AVPixelFormat)stream->codecpar->format;
                    wantIndex.video_bit_rate = stream->codecpar->bit_rate;
                    wantIndex.fps = stream->r_frame_rate.num;
                }
                
                dectx.video_de_ctx = ctx;
            }
            
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                index.audio_index = j;
                index.sample_rate = stream->codecpar->sample_rate;
                index.ch_layout = stream->codecpar->channel_layout;
                index.audio_codecId = stream->codecpar->codec_id;
                index.smp_fmt = (enum AVSampleFormat)stream->codecpar->format;
                index.audio_bit_rate = stream->codecpar->bit_rate;
                
                // 输出文件的音频编码标准
                if (wantIndex.audio_codecId == AV_CODEC_ID_NONE) {  // 取第一个出现的值
                    wantIndex.audio_index = j;
                    wantIndex.sample_rate = stream->codecpar->sample_rate;
                    wantIndex.ch_layout = stream->codecpar->channel_layout;
                    wantIndex.audio_codecId = stream->codecpar->codec_id;
                    wantIndex.smp_fmt = (enum AVSampleFormat)stream->codecpar->format;
                    wantIndex.audio_bit_rate = stream->codecpar->bit_rate;
                }
                dectx.audio_de_ctx = ctx;
            }
        }
        in_indexes.push_back(index);
        de_ctxs.push_back(dectx);
    }
    
    return true;
}

static int select_sample_rate(AVCodec *codec,int rate)
{
    int best_rate = 0;
    int deft_rate = 44100;
    bool surport = false;
    const int* p = codec->supported_samplerates;
    if (!p) {
        return deft_rate;
    }
    while (*p) {
        best_rate = *p;
        if (*p == rate) {
            surport = true;
            break;
        }
        p++;
    }
    
    if (best_rate != rate && best_rate != 0 && best_rate != deft_rate) {
        return deft_rate;
    }
    
    return best_rate;
}

bool Merge::addStream()
{
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstpath.c_str()) < 0) {
        LOGD("avformat_alloc_out_context2 fail");
        releasesources();
        return false;
    }
    
    if (wantIndex.video_codecId != AV_CODEC_ID_NONE) {
        
        // 添加视频流
        AVStream *stream = avformat_new_stream(ou_fmt, NULL);
        video_ou_index = stream->index;
        
        AVCodec *codec = avcodec_find_encoder(wantIndex.video_codecId);
        AVCodecContext *ctx = avcodec_alloc_context3(codec);
        if (ctx == NULL) {
            LOGD("find ctx fail");
            releasesources();
            return false;
        }
        // 设置编码参数
        ctx->pix_fmt = wantIndex.pix_fmt;
        ctx->width = wantIndex.width;
        ctx->height = wantIndex.height;
        ctx->bit_rate = wantIndex.video_bit_rate;
        ctx->time_base = (AVRational){1,wantIndex.fps*100};
        ctx->framerate = (AVRational){wantIndex.fps,1};
        stream->time_base = ctx->time_base;
        
        if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
            ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        if (codec->id == AV_CODEC_ID_H264) {
            ctx->flags2 |= AV_CODEC_FLAG2_LOCAL_HEADER;
        }
        
        // 初始化上下文
        if (avcodec_open2(ctx, codec, NULL) < 0) {
            LOGD("avcodec_open2 fail");
            releasesources();
            return false;
        }
        
        if (avcodec_parameters_from_context(stream->codecpar, ctx) < 0) {
            LOGD("avcodec_parameters_from_context fail");
            releasesources();
            return false;
        }
        
        video_en_ctx = ctx;
    }
    
    if (wantIndex.audio_codecId != AV_CODEC_ID_NONE) {
        AVStream *stream = avformat_new_stream(ou_fmt, NULL);
        audio_ou_index = stream->index;
        AVCodec *codec = avcodec_find_encoder(wantIndex.audio_codecId);
        AVCodecContext *ctx = avcodec_alloc_context3(codec);
        if (ctx == NULL) {
            LOGD("avcodec alloc fail");
            releasesources();
            return false;
        }
        // 设置音频编码参数
        ctx->sample_fmt = wantIndex.smp_fmt;
        ctx->channel_layout = wantIndex.ch_layout;
        int relt_sample_rate = select_sample_rate(codec, wantIndex.sample_rate);
        if (relt_sample_rate == 0) {
            LOGD("cannot surpot sample_rate");
            releasesources();
            return false;
        }
        ctx->sample_rate = relt_sample_rate;
        ctx->bit_rate = wantIndex.audio_bit_rate;
        ctx->time_base = (AVRational){1,ctx->sample_rate};
        stream->time_base = ctx->time_base;
        if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
            ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        // 初始化编码器
        if (avcodec_open2(ctx, codec, NULL) < 0) {
            LOGD("aovcec_open2() fail");
            releasesources();
            return false;
        }
        if (avcodec_parameters_from_context(stream->codecpar, ctx) < 0) {
            LOGD("avcodec_parameters_from_context fail");
            releasesources();
            return false;
        }
        
        audio_en_ctx = ctx;
    }
    
    // 打开输入上下文及写入头信息
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2 fail");
            releasesources();
            return false;
        }
    }
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header fail");
        releasesources();
        return false;
    }
    
    
    return true;
}

void Merge::updateAVFrame()
{
    if (video_en_frame) {
        av_frame_unref(video_en_frame);
    }
    if (audio_en_frame) {
        av_frame_unref(audio_en_frame);
    }
    if (video_en_ctx) {
        video_en_frame = av_frame_alloc();
        video_de_frame = av_frame_alloc();
        video_en_frame->format = video_en_ctx->pix_fmt;
        video_en_frame->width = video_en_ctx->width;
        video_en_frame->height = video_en_ctx->height;
        av_frame_get_buffer(video_en_frame, 0);
        av_frame_make_writable(video_en_frame);
    }
    
    if (audio_en_ctx) {
        audio_en_frame = av_frame_alloc();
        audio_de_frame = av_frame_alloc();
        audio_en_frame->format = audio_en_ctx->sample_fmt;
        audio_en_frame->channel_layout = audio_en_ctx->channel_layout;
        audio_en_frame->sample_rate = audio_en_ctx->sample_rate;
        audio_en_frame->nb_samples = audio_en_ctx->frame_size;
        av_frame_get_buffer(audio_en_frame, 0);
        av_frame_make_writable(audio_en_frame);
    }
}

void Merge::doDecode(AVPacket *pkt,bool isVideo)
{
    DeCtx dectx = de_ctxs[curIndex.file_index];
    AVCodecContext *ctx = NULL;
    AVFrame *de_frame = NULL;
    AVFrame *en_frame = NULL;
    if (isVideo) {   // 说明是视频
        ctx = dectx.video_de_ctx;
        de_frame = video_de_frame;
        en_frame = video_en_frame;
    } else {    // 说明是音频
        ctx = dectx.audio_de_ctx;
        de_frame = audio_de_frame;
        en_frame = audio_en_frame;
        isVideo = false;
    }
    if (ctx == NULL) {
        LOGD("%d error avcodecxtx",curIndex);
        releasesources();
        return;
    }
    
    // 开始解码
    int ret = 0;
    avcodec_send_packet(ctx, pkt);
    while (true) {
        ret = avcodec_receive_frame(ctx, de_frame);
        if (ret == AVERROR_EOF && curIndex.file_index + 1 == in_fmts.size()) {   // 说明解码完毕
            doEncode(NULL,ctx->width > 0);
            LOGD("decode finish");
            break;
        } else if(ret < 0) {
            break;
        }
        
        // 解码成功;开始编码
        doConvert(&en_frame, de_frame,de_frame->width > 0);
    }
}

void Merge::doConvert(AVFrame **dst, AVFrame *src,bool isVideo)
{
    int ret = 0;
    if (isVideo && (src->width != (*dst)->width || src->format != (*dst)->format)) {
        MediaIndex index = in_indexes[curIndex.file_index];
        if (!swsctx) {
            swsctx = sws_getContext(index.width, index.height, index.pix_fmt,
                                    video_en_ctx->width, video_en_ctx->height, video_en_ctx->pix_fmt,
                                    0, NULL, NULL, NULL);
            if (swsctx == NULL) {
                LOGD("sws_getContext fail");
                releasesources();
                return;
            }
        }
        
        
        if((ret = sws_scale(swsctx, src->data, src->linesize, 0, src->height, (*dst)->data, (*dst)->linesize)) < 0) {
            LOGD("sws_scale fail");
            releasesources();
            return;
        }
        
        /** 遇到问题：合成后的视频时长不是各个视频文件视频时长之和
         *  分析原因：因为每个视频文件的fps不一样，合并时pts没有和每个视频文件的fps对应
         *  解决方案：合并时pts要和每个视频文件的fps对应
         */
        (*dst)->pts = next_video_pts + video_en_ctx->time_base.den/curIndex.fps;
        next_video_pts += video_en_ctx->time_base.den/curIndex.fps;
        doEncode((*dst),(*dst)->width > 0);
        
    } else if (isVideo && (src->width == (*dst)->width && src->format == (*dst)->format)) {
        av_frame_copy((*dst), src);
        (*dst)->pts = next_video_pts + video_en_ctx->time_base.den/curIndex.fps;
        next_video_pts += video_en_ctx->time_base.den/curIndex.fps;
        doEncode((*dst),(*dst)->width > 0);
    }
    
    if (!isVideo && (src->sample_rate != (*dst)->sample_rate || src->format != (*dst)->format || src->channel_layout != (*dst)->channel_layout)) {
        
        if (!swrctx) {
            swrctx = swr_alloc_set_opts(NULL, audio_en_ctx->channel_layout, audio_en_ctx->sample_fmt, audio_en_ctx->sample_rate,
                                        src->channel_layout, (enum AVSampleFormat)src->format, src->sample_rate, 0, NULL);
            if ((ret = swr_init(swrctx)) < 0) {
                LOGD("swr_alloc_set_opts() fail %d",ret);
                releasesources();
                return;
            }
        }
        
        int dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swrctx, src->sample_rate)+src->nb_samples, (*dst)->sample_rate, src->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples != (*dst)->nb_samples) {
            
            // 进行转化
            /** 遇到问题：音频合并后时长并不是两个文件之和
             *  分析原因：当进行音频采样率重采样时，如采样率升采样，那么swr_convert()内部会进行插值运算，这样相对于原先的nb_samples会多出一些，所以swr_convert()可能需要多次
             *  调用才可以将所有数据取完，那么这就需要建立一个缓冲区来自己组装新采样率下的AVFrame中的音频数据
             *  解决方案：建立音频缓冲区，依次组装所有的音频数据并且和pts对应上
             */
            if (!audio_init) {
                ret = av_samples_alloc_array_and_samples(&audio_buffer, (*dst)->linesize, (*dst)->channels, (*dst)->nb_samples, (enum AVSampleFormat)(*dst)->format, 0);
                audio_init = true;
                left_size = 0;
            }
            bool first = true;
            while (true) {
                // 进行转换
                if (first) {
                    ret = swr_convert(swrctx, audio_buffer, (*dst)->nb_samples, (const uint8_t**)audio_de_frame->data, audio_de_frame->nb_samples);
                    if (ret < 0) {
                        LOGD("swr_convert() fail %d",ret);
                        releasesources();
                        return;
                    }
                    first = false;
                    
                    int use = ret-left_size >= 0 ?ret - left_size:ret;
                    int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                    for (int ch=0; ch<audio_en_frame->channels; ch++) {
                        for (int i = 0; i<use; i++) {
                            (*dst)->data[ch][(i+left_size)*size] = audio_buffer[ch][i*size];
                            (*dst)->data[ch][(i+left_size)*size+1] = audio_buffer[ch][i*size+1];
                            (*dst)->data[ch][(i+left_size)*size+2] = audio_buffer[ch][i*size+2];
                            (*dst)->data[ch][(i+left_size)*size+3] = audio_buffer[ch][i*size+3];
                        }
                    }
                    // 编码
                    left_size += ret;
                    if (left_size >= (*dst)->nb_samples) {
                        left_size -= (*dst)->nb_samples;
                        // 编码
                        (*dst)->pts = av_rescale_q(next_audio_pts, (AVRational){1,(*dst)->sample_rate}, audio_en_ctx->time_base);
                        next_audio_pts += (*dst)->nb_samples;
                        doEncode((*dst),(*dst)->width > 0);
                        
                        if (left_size > 0) {
                            int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                            for (int ch=0; ch<(*dst)->channels; ch++) {
                                for (int i = 0; i<left_size; i++) {
                                    (*dst)->data[ch][i*size] = audio_buffer[ch][(use+i)*size];
                                    (*dst)->data[ch][i*size+1] = audio_buffer[ch][(use+i)*size+1];
                                    (*dst)->data[ch][i*size+2] = audio_buffer[ch][(use+i)*size+2];
                                    (*dst)->data[ch][i*size+3] = audio_buffer[ch][(use+i)*size+3];
                                }
                            }
                        }
                    }
                    
                } else {
                    ret = swr_convert(swrctx, audio_buffer, (*dst)->nb_samples, NULL, 0);
                    if (ret < 0) {
                        LOGD("1 swr_convert() fail %d",ret);
                        releasesources();
                        return;
                    }
                    int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                    for (int ch=0; ch<audio_en_frame->channels; ch++) {
                        for (int i = 0; i < ret && i+left_size < audio_en_frame->nb_samples; i++) {
                            (*dst)->data[ch][(left_size + i)*size] = audio_buffer[ch][i*size];
                            (*dst)->data[ch][(left_size + i)*size + 1] = audio_buffer[ch][i*size + 1];
                            (*dst)->data[ch][(left_size + i)*size + 2] = audio_buffer[ch][i*size + 2];
                            (*dst)->data[ch][(left_size + i)*size + 3] = audio_buffer[ch][i*size + 3];
                        }
                    }
                    left_size += ret;
                    if (left_size >= (*dst)->nb_samples) {
                        left_size -= (*dst)->nb_samples;
                        LOGD("多了一个编码");
                        // 编码
                        (*dst)->pts = av_rescale_q(next_audio_pts, (AVRational){1,(*dst)->sample_rate}, audio_en_ctx->time_base);
                        next_audio_pts += audio_en_frame->nb_samples;
                        doEncode((*dst),(*dst)->width > 0);
                    } else {
                        break;
                    }
                }
            }
        }
        
    } else if (!isVideo && (src->nb_samples == (*dst)->nb_samples || src->sample_rate == (*dst)->sample_rate || src->format == (*dst)->format || src->channel_layout == (*dst)->channel_layout)) {
        av_frame_copy((*dst), src);
        /** 遇到问题：合并后音频和视频不同步
         *  分析原因：因为每个音频文件的采样率，而合并是按照第一个文件的采样率作为time_base的，没有转换过来
         *  解决方案：合并时pts要和每个合并前音频的采样率对应上
         */
        src->pts = next_audio_pts;
        next_audio_pts = src->pts + audio_en_ctx->time_base.den/curIndex.sample_rate * src->nb_samples;
        LOGD("next_audio_pts %d",next_audio_pts);
        doEncode(src,src->width > 0);
    }
}

AVFrame* Merge::get_audio_frame(enum AVSampleFormat smpfmt,int64_t ch_layout,int sample_rate,int nb_samples)
{
    AVFrame * audio_en_frame = av_frame_alloc();
    // 根据采样格式，采样率，声道类型以及采样数分配一个AVFrame
    audio_en_frame->sample_rate = sample_rate;
    audio_en_frame->format = smpfmt;
    audio_en_frame->channel_layout = ch_layout;
    audio_en_frame->nb_samples = nb_samples;
    int ret = 0;
    if ((ret = av_frame_get_buffer(audio_en_frame, 0)) < 0) {
        LOGD("audio get frame buffer fail %d",ret);
        return NULL;
    }

    if ((ret =  av_frame_make_writable(audio_en_frame)) < 0) {
        LOGD("audio av_frame_make_writable fail %d",ret);
        return NULL;
    }
    
    return audio_en_frame;
}

void Merge::doEncode(AVFrame *frame,bool isVideo)
{
    
    AVCodecContext *ctx = NULL;
    if (isVideo) {  // 说明是视频
        ctx = video_en_ctx;
    } else {
        ctx = audio_en_ctx;
    }
    
    int ret = avcodec_send_frame(ctx, frame);
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        ret = avcodec_receive_packet(ctx, pkt);
        if (ret < 0) {
//            LOGD("ret %s",av_err2str(ret));
            av_packet_unref(pkt);
            break;
        }
        
        // 编码成功;写入文件,写入之前改变一下pts,dts和duration(因为此时pkt的pts是基于AVCodecContext的，要转换成基于AVFormatContext的)
        int index = ctx->width > 0?video_ou_index:audio_ou_index;
        AVStream *stream = ou_fmt->streams[index];
        av_packet_rescale_ts(pkt, ctx->time_base, stream->time_base);
        pkt->stream_index = index;
        LOGD("%s pts %d(%s) dts %d(%s)",pkt->stream_index == video_ou_index ? "video":"audio",pkt->pts,av_ts2timestr(pkt->pts,&stream->time_base),pkt->pts,av_ts2timestr(pkt->dts,&stream->time_base));
        if ((ret = av_write_frame(ou_fmt, pkt)) < 0) {
            LOGD("av_write_frame fail %d",ret);
            releasesources();
            return;
        }
        av_packet_unref(pkt);
    }
}

void Merge::addMusic(string srcpath,string srcpath2,string dstpath,string st)
{
    /** 遇到问题：如果添加的音频文件为Mp3文件，则使用苹果的内置播放器及苹果手机没有声音。如果添加的是aac音频 则没有问题
     *  分析原因：暂时未知
     *  解决方案：暂时未知
     */
    string start = st;
    if (st.length() <= 8) {
        start = "00:00:05";
    }
    start_pos = 0;
    start_pos += stoi(start.substr(0,2))*3600;
    start_pos += stoi(start.substr(3,2))*60;
    start_pos += stoi(start.substr(6,2));
    
    in_fmt1 = NULL;// 用于解封装视频
    in_fmt2 = NULL;// 用于解封装音频
    ou_fmt = NULL;  //用于封装音视频
    
    // 打开视频文件
    if (avformat_open_input(&in_fmt1, srcpath.c_str(), NULL, NULL) < 0) {
        LOGD("1 avformat_open_input() fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt1, NULL) < 0) {
        LOGD("1 avformat_find_stream_info");
        releasesources();
        return;
    }
    
    // 打开音频文件
    if (avformat_open_input(&in_fmt2, srcpath2.c_str(), NULL, NULL) < 0) {
        LOGD("2 avformat_open_input() fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt2, NULL) < 0) {
        LOGD("2 avformat_find_stream_info");
        releasesources();
        return;
    }
    for (int i=0; i<in_fmt1->nb_streams; i++) {
        AVStream *stream = in_fmt1->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video1_in_index = i;
            break;
        }
    }
    for (int i = 0; i<in_fmt2->nb_streams; i++) {
        AVStream *stream = in_fmt2->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio1_in_index = i;
            break;
        }
    }
    
    // 打开输出文件的封装器
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstpath.c_str()) < 0) {
        LOGD("avformat_alloc_out_context2 ()");
        releasesources();
        return;
    }
    
    // 添加视频流并从输入源拷贝视频流编码参数
    AVStream *stream = avformat_new_stream(ou_fmt, NULL);
    video_ou_index = stream->index;
    if (avcodec_parameters_copy(stream->codecpar, in_fmt1->streams[video1_in_index]->codecpar) < 0) {
        releasesources();
        return;
    }
    // 如果源和目标文件的码流格式不一致，则将目标文件的code_tag赋值为0
    if (av_codec_get_id(ou_fmt->oformat->codec_tag, in_fmt1->streams[video1_in_index]->codecpar->codec_tag) != stream->codecpar->codec_id) {
        stream->codecpar->codec_tag = 0;
    }
    // 添加音频流并从输入源拷贝编码参数
    AVStream *a_stream = avformat_new_stream(ou_fmt, NULL);
    audio_ou_index = a_stream->index;
    if (avcodec_parameters_copy(a_stream->codecpar, in_fmt2->streams[audio1_in_index]->codecpar) < 0) {
        LOGD("avcodec_parameters_copy fail");
        releasesources();
        return;
    }
    if (av_codec_get_id(ou_fmt->oformat->codec_tag, in_fmt2->streams[audio1_in_index]->codecpar->codec_tag) != a_stream->codecpar->codec_id) {
        a_stream->codecpar->codec_tag = 0;
    }
    
    // 打开输出上下文
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2() fail");
            releasesources();
            return;
        }
    }
    
    // 写入头文件
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header()");
        releasesources();
        return;
    }
    

    AVPacket *v_pkt = av_packet_alloc();
    AVPacket *a_pkt = av_packet_alloc();
    // 写入视频
    int64_t video_max_pts = 0;
    while (av_read_frame(in_fmt1, v_pkt) >= 0) {
        if (v_pkt->stream_index == video1_in_index) {   // 说明是视频
            // 因为源文件和目的文件时间基可能不一致，所以这里要进行转换
            av_packet_rescale_ts(v_pkt, in_fmt1->streams[video1_in_index]->time_base, ou_fmt->streams[video_ou_index]->time_base);
            v_pkt->stream_index = video_ou_index;
            video_max_pts = max(v_pkt->pts, video_max_pts);
            LOGD("video pts %d(%s)",v_pkt->pts,av_ts2timestr(v_pkt->pts, &stream->time_base));
            if (av_write_frame(ou_fmt, v_pkt) < 0) {
                LOGD("1 av_write_frame < 0");
                releasesources();
                return;
            }
        }
    }

    int64_t start_pts = start_pos * a_stream->time_base.den;
    video_max_pts = av_rescale_q(video_max_pts, stream->time_base, a_stream->time_base);
    // 写入音频
    while (av_read_frame(in_fmt2, a_pkt) >= 0) {
        if (a_pkt->stream_index == audio1_in_index) {   // 音频
    
            // 源文件和目标文件的时间基可能不一致，需要转化
            av_packet_rescale_ts(a_pkt, in_fmt2->streams[audio1_in_index]->time_base, ou_fmt->streams[audio_ou_index]->time_base);
            // 保证以视频时间轴的指定时间进行添加，那么实际上就是改变pts及dts的值即可
            a_pkt->pts += start_pts;
            a_pkt->dts += start_pts;
            a_pkt->stream_index = audio_ou_index;
            LOGD("audio pts %d(%s)",a_pkt->pts,av_ts2timestr(a_pkt->pts, &a_stream->time_base));
            // 加入音频的时长不能超过视频的总时长
            if (a_pkt->pts >= video_max_pts) {
                break;
            }
            
            if (av_write_frame(ou_fmt, a_pkt) < 0) {
                LOGD("2 av_write_frame < 0");
                releasesources();
                return;
            }
        }
    }
    
    av_write_trailer(ou_fmt);
    LOGD("写入完毕");
    
    // 释放资源
    releasesources();
    
}


void Merge::releasesources()
{
    if (in_fmt1) {
        avformat_close_input(&in_fmt1);
        in_fmt1 = NULL;
    }
    
    if (in_fmt2) {
        avformat_close_input(&in_fmt2);
        in_fmt2 = NULL;
    }
    
    if (ou_fmt) {
        avformat_free_context(ou_fmt);
        ou_fmt = NULL;
    }
    
    for (int i=0; i<in_fmts.size(); i++) {
        AVFormatContext *fmt = in_fmts[i];
        avformat_close_input(&fmt);
    }
    
    for (int i = 0; i<de_ctxs.size(); i++) {
        DeCtx ctx = de_ctxs[i];
        if (ctx.video_de_ctx) {
            avcodec_free_context(&ctx.video_de_ctx);
            ctx.video_de_ctx = NULL;
        }
        if (ctx.audio_de_ctx) {
            avcodec_free_context(&ctx.audio_de_ctx);
            ctx.audio_de_ctx = NULL;
        }
    }
    
    if (video_en_ctx) {
        avcodec_free_context(&video_en_ctx);
        video_en_ctx = NULL;
    }
    
    if (audio_en_ctx) {
        avcodec_free_context(&audio_en_ctx);
        audio_en_ctx = NULL;
    }
    
    if (swsctx) {
        sws_freeContext(swsctx);
        swsctx = NULL;
    }
    
    if (swrctx) {
        swr_free(&swrctx);
        swrctx = NULL;
    }
    
    if (video_de_frame) {
        av_frame_free(&video_de_frame);
    }
    
    if (audio_de_frame) {
        av_frame_free(&audio_de_frame);
    }
    if (video_en_frame) {
        av_frame_free(&video_en_frame);
    }
    if (audio_en_frame) {
        av_frame_free(&audio_en_frame);
    }
    if (audio_buffer) {
        av_freep(&audio_buffer[0]);
    }
}
