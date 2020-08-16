//
//  Cut.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/18.
//  Copyright © 2020 apple. All rights reserved.
//

#include "Cut.hpp"

void Cut::releasesources()
{
    if (in_fmtctx) {
        avformat_close_input(&in_fmtctx);
        in_fmtctx = NULL;
    }
    if (ou_fmtctx) {
        avformat_free_context(ou_fmtctx);
        ou_fmtctx = NULL;
    }
    if (video_en_frame) {
        av_frame_unref(video_en_frame);
        video_en_frame = NULL;
    }
    if (video_de_ctx) {
        avcodec_free_context(&video_de_ctx);
        video_de_ctx = NULL;
    }
    if (video_en_ctx) {
        avcodec_free_context(&video_en_ctx);
        video_de_ctx = NULL;
    }
}

void Cut::doWrite(AVPacket *pkt)
{
    
    if (!has_writer_header) {
        
        if (video_en_ctx == NULL) { //  说明没有进行过编码
            // 拷贝源文件视频频编码参数
            AVStream *au_stream = ou_fmtctx->streams[video_ou_tream_index];
            if (avcodec_parameters_copy(au_stream->codecpar,in_fmtctx->streams[video_in_stream_index]->codecpar) < 0) {
                LOGD("avcodec_parameters_copy fail");
                releasesources();
                return;
            }
        }
        
        // 拷贝源文件音频编码参数
        AVStream *au_stream = ou_fmtctx->streams[audio_ou_stream_index];
        if (avcodec_parameters_copy(au_stream->codecpar, in_fmtctx->streams[audio_in_stream_index]->codecpar) < 0) {
            LOGD("avcodec_parameters_copy fail");
            releasesources();
            return;
        }
        
        // 打开io上下文
        if (!(ou_fmtctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open2(&ou_fmtctx->pb, dstPath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
                LOGD("avio_open2() fail");
                releasesources();
                return;
            }
        }
                
        // 写入文件头
        if (avformat_write_header(ou_fmtctx, NULL) < 0) {
            LOGD("avformat_write_header()");
            releasesources();
            return;
        }
        
        has_writer_header = true;
    }
    
    
    int ret = 0;
    if (pkt->stream_index == video_ou_tream_index) {
        if (video_en_ctx != NULL) {
            // 写入之前重新转换一下时间戳
            av_packet_rescale_ts(pkt, video_en_ctx->time_base, ou_fmtctx->streams[video_ou_tream_index]->time_base);
        } else {
            av_packet_rescale_ts(pkt, in_fmtctx->streams[video_in_stream_index]->time_base, ou_fmtctx->streams[video_ou_tream_index]->time_base);
        }
        
    } else {
        // 进行时间戳的转换
        av_packet_rescale_ts(pkt, in_fmtctx->streams[audio_in_stream_index]->time_base, ou_fmtctx->streams[audio_ou_stream_index]->time_base);
    }
    
    LOGD("%s pts %ld(%s)",pkt->stream_index == video_ou_tream_index?"vi":"au",pkt->pts,av_ts2timestr(pkt->pts, &ou_fmtctx->streams[pkt->stream_index]->time_base));
    if((ret = av_write_frame(ou_fmtctx, pkt)) < 0) {
        LOGD("av_write_frame fail %d",ret);
        releasesources();
        return;
    }
}

void Cut::doDecode(AVPacket *inpkt)
{
    AVCodecContext *decodec_ctx = video_de_ctx;
    AVFormatContext *infmt = in_fmtctx;
    
    // 初始化解码器
    AVStream *stream = infmt->streams[video_in_stream_index];
    if (decodec_ctx == NULL) {
        AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            LOGD("avcodec_find_decoder fail");
            releasesources();
            return;
        }
        decodec_ctx = avcodec_alloc_context3(codec);
        if (!decodec_ctx) {
            LOGD("avcodec_alloc_context3() fail");
            releasesources();
            return;
        }
        
        // 设置解码参数;这里直接从对应的AVStream中拷贝过来
        if (avcodec_parameters_to_context(decodec_ctx, stream->codecpar) < 0) {
            LOGD("avcodec_parameters_from_context fail");
            releasesources();
            return;
        }
        
        // 初始化解码器上下文
        if (avcodec_open2(decodec_ctx, codec, NULL) < 0) {
            LOGD("avcodec_open2() fail");
            releasesources();
            return;
        }
        
        // 进行赋值
        video_de_ctx = decodec_ctx;
    }
    
    // 初始化编码器
    if (video_en_ctx == NULL) {
        
        AVCodec *codec = avcodec_find_encoder(video_de_ctx->codec_id);
        if (!codec) {
            LOGD("avcodec avcodec_find_encoder fail");
            releasesources();
            return;
        }
        video_en_ctx = avcodec_alloc_context3(codec);
        if (video_en_ctx == NULL) {
            LOGD("avcodec_alloc_context3 fail");
            releasesources();
            return;
        }
        
        // 设置编码参数;由于不做任何编码方式的改变，这里直接从源拷贝。
        AVStream *stream = in_fmtctx->streams[video_in_stream_index];
        if (avcodec_parameters_to_context(video_en_ctx, stream->codecpar) < 0) {
            LOGD("avcodec_parameters_to_context fail");
            releasesources();
            return;
        }
        // 设置时间基
        video_en_ctx->time_base = stream->time_base;
        video_en_ctx->framerate = stream->r_frame_rate;
        
        // 必须要设置，否则如mp4文件生成后无法看到预览图
        if (ou_fmtctx->oformat->flags & AVFMT_GLOBALHEADER) {
            video_en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }
        
        // 初始化编码器
        if(avcodec_open2(video_en_ctx,codec,NULL) < 0) {
            LOGD("avcodec_open2() fail");
            releasesources();
            return;
        }
        
        // 设置输出视频流的对应的编码参数
        AVStream *ou_stream = ou_fmtctx->streams[video_ou_tream_index];
        // 设置相关参数
        if(avcodec_parameters_from_context(ou_stream->codecpar, video_en_ctx) < 0) {
            LOGD("avcodec_parameters_from_context fail");
            releasesources();
            return;
        }
    }
    
    if (video_de_frame == NULL) {
        video_de_frame = av_frame_alloc();
    }
    
    // 进行解码
    int ret = 0;
    ret = avcodec_send_packet(decodec_ctx, inpkt);
//    if (inpkt) {
//        LOGD("inpkt pts %d(%s)",inpkt->pts,av_ts2timestr(inpkt->pts,&stream->time_base));
//    }
    while (true) {
        
        ret = avcodec_receive_frame(decodec_ctx, video_de_frame);
        if (ret == AVERROR_EOF) {
            // 解码完毕
            doEncode(NULL);
            break;
        } else if(ret < 0){
            break;
        }
        
        // 解码了一帧数据；解码得到的AVFrame和前面送往解码器的AVpacket是一一对应的，所以两者的pts也是对应的
//        LOGD("deframe pts %d(%s)",(video_de_frame)->pts,av_ts2timestr((video_de_frame)->pts,&stream->time_base));
        if (video_de_frame->pts >= video_start_pts) {
            // 进行编码;编码之前进行数据的重新拷贝
            doEncode(video_de_frame);
        }
    }
}

void Cut::doEncode(AVFrame *enfram)
{
    // 开始进行编码
    if (video_en_frame == NULL) {
        video_en_frame = av_frame_alloc();
        video_en_frame->width = video_en_ctx->width;
        video_en_frame->height = video_en_ctx->height;
        video_en_frame->format = video_en_ctx->pix_fmt;
        av_frame_get_buffer(video_en_frame, 0);
        if(av_frame_make_writable(video_en_frame) < 0) {
            LOGD("av_frame_make_writable fail");
            releasesources();
            return;
        }
    }
    
    // 将要编码的数据拷贝过来
    int ret = 0;
    AVPacket *pkt = av_packet_alloc();
    if (enfram) {
        av_frame_copy(video_en_frame, enfram);
        video_en_frame->pts = video_next_pts * video_en_ctx->time_base.den/video_en_ctx->framerate.num;
           video_next_pts++;
        avcodec_send_frame(video_en_ctx, video_en_frame);
    } else {
        avcodec_send_frame(video_en_ctx, NULL);
    }
    
    while (true) {
        ret = avcodec_receive_packet(video_en_ctx, pkt);
        if (ret < 0) {
            break;
        }
        
        // 编码成功;写入文件
        LOGD("encode pts %ld(%s)",pkt->pts,av_ts2timestr(pkt->pts, &(in_fmtctx->streams[video_in_stream_index]->time_base)));
        pkt->stream_index = video_ou_tream_index;
        doWrite(pkt);
    }
    
}

void Cut::doCut(string spath,string dpath,string start,int du)
{
    // 只考虑同一种容器的内容剪切
    srcPath = spath;
    dstPath = dpath;
    
    // 截取起始时间 格式 hh:mm:ss
    if (start.length() < 8) {
        start = "00:00:15";
    }
    duration = du;
    if (du <= 0) {
        duration = 5;        // 时长 单位秒
    }
    start_pos = 0;
    video_next_pts = 0;
    video_start_pts = 0;
    audio_start_pts = 0;
    
    
    // 那么最终截取的文件长度将从start处开始之后的duration秒;
    start_pos += stoi(start.substr(0,2))*3600;
    start_pos += stoi(start.substr(3,2))*60;
    start_pos += stoi(start.substr(6,2));
    
    has_writer_header = false;
    in_fmtctx = NULL;
    ou_fmtctx = NULL;
    video_de_frame = NULL;
    video_en_frame = NULL;
    video_de_ctx = NULL;
    video_en_ctx = NULL;
    
    int ret = 0;
    audio_in_stream_index = -1;
    video_in_stream_index = -1;
    audio_ou_stream_index = -1;
    video_ou_tream_index = -1;
    if ((ret = avformat_open_input(&in_fmtctx, srcPath.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input fail");
        releasesources();
        return;
    }
    if ((ret = avformat_find_stream_info(in_fmtctx, NULL)) < 0) {
        LOGD("avformat_find_stream_info fail");
        releasesources();
        return;
    }
    
    // 获取输入流的索引
    for (int i=0; i<in_fmtctx->nb_streams; i++) {
        
        AVStream *stream = in_fmtctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_in_stream_index = i;
        }
        
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_in_stream_index = i;
        }
    }
    
    // 将视频流指针移到指定的位置，那么后面调用av_read_frame()函数时将从此位置开始读取
    if (video_in_stream_index != -1) {
        AVStream *stream = in_fmtctx->streams[video_in_stream_index];
        /** 移动当前读取指针到指定的位置
         *  参数2：指定移动的流索引，可以为-1，如果为-1那么将选择in_fmtctx的默认索引
         *  参数3：指定移动的时间位置(时间戳)，基于指定流的时间基。如果参数二为-1，那么这里基于AV_TIME_BASE时间基。
         *  参数4：指定移动的算法;AVSEEK_FLAG_BACKWARD：代表基于参数3指定的时间戳往前找，直到找到I帧。
         */
        if((ret = av_seek_frame(in_fmtctx,video_in_stream_index,start_pos*stream->time_base.den/stream->time_base.num,AVSEEK_FLAG_BACKWARD)) < 0){
            LOGD("vdio av_seek_frame fail %d",ret);
            releasesources();
            return;
        }
    }

    // 将音频流指针移到指定的位置
    if (audio_in_stream_index != -1) {
        AVStream *stream = in_fmtctx->streams[audio_in_stream_index];
        if ((ret = av_seek_frame(in_fmtctx, audio_in_stream_index, start_pos*stream->time_base.den/stream->time_base.num, AVSEEK_FLAG_BACKWARD) < 0)) {
            LOGD("audio av_seek_frame %d",ret);
            releasesources();
            return;
        }
    }
    
    // 打开封装器
    if(avformat_alloc_output_context2(&ou_fmtctx, NULL, NULL, dstPath.c_str()) < 0) {
        LOGD("avformat_alloc_output_context2 fail");
        releasesources();
        return;
    }
    
    // 添加对应的音视频流;
    if (video_in_stream_index != -1) {
        AVStream *stream = avformat_new_stream(ou_fmtctx, NULL);
        video_ou_tream_index = stream->index;
    }
            
    if (audio_in_stream_index != -1) {
        AVStream *stream = avformat_new_stream(ou_fmtctx, NULL);
        audio_ou_stream_index = stream->index;
    }
    
    
    // 读取AVPacket
    AVPacket *in_packt = av_packet_alloc();
    bool first_video_packet = true;
    bool video_need_decode = true;
    bool flag = false;
    bool video_end = false;
    bool audio_end = false;
    while (av_read_frame(in_fmtctx, in_packt) >= 0) {
        
        // 达到时间点了 则终止
        if (video_end && audio_end) {
            break;
        }
        
        AVStream *stream = in_fmtctx->streams[in_packt->stream_index];
        if (in_packt->stream_index == video_in_stream_index) {
            // 对于视频来说，起始时刻对应的AVPacket可能不是I帧，所以就需要先解码再编码
            
            // 允许有5帧的时间差
            int64_t max_frame = 5;
            int64_t delt_pts = av_rescale_q(max_frame, (AVRational){1,stream->r_frame_rate.num},stream->time_base);
            video_start_pts = start_pos * stream->time_base.den/stream->time_base.num;
            int64_t end_pts = (start_pos + duration) * stream->time_base.den/stream->time_base.num;
            if (first_video_packet && video_start_pts - in_packt->pts <= delt_pts) {
                video_need_decode = false;
                LOGD("is key frame pts %s",av_ts2timestr(in_packt->pts,&stream->time_base));
            }
            
            if (end_pts <=in_packt->pts) {
                doDecode(NULL);
                video_end = true;
                continue;
            }
            

            if (video_need_decode) {    // 如果需要重新编解码 则进入重新编解码的流程
                doDecode(in_packt);
            } else {
                in_packt->pts -= video_start_pts;
                in_packt->dts -= video_start_pts;
                in_packt->stream_index = video_ou_tream_index;
                doWrite(in_packt);
            }
            
            first_video_packet = false;
            flag = true;
        }
        
        // 对于音频来说
        if (in_packt->stream_index == audio_in_stream_index && flag) {
            audio_start_pts = (start_pos) * stream->time_base.den/stream->time_base.num;
            int64_t end_pts = (start_pos + duration) * stream->time_base.den / stream->time_base.num;
            if (end_pts <= in_packt->pts) {
                audio_end = true;
                continue;;
            }
            /** 遇到问题：音频正常播放，视频只是播放了很短的几帧画面
             *  分析原因：由于pkt的时间戳没有减去起始时间导致了音视频的时间戳错乱
             *  解决方案：音频时间戳减去起始时间即可
             */
            in_packt->pts -= audio_start_pts;
            in_packt->dts -= audio_start_pts;
            in_packt->stream_index = audio_ou_stream_index;
            doWrite(in_packt);
        }
    }
    
    // 写入文件尾部
    av_write_trailer(ou_fmtctx);
    LOGD("结束写入文件..");
    // 释放资源
    releasesources();
}
