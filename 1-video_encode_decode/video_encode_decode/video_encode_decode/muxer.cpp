//
//  muxer.cpp
//  video_encode_decode
//
//  Created by apple on 2020/4/14.
//  Copyright © 2020 apple. All rights reserved.
//

#include "muxer.hpp"

Muxer::Muxer()
{
    
}
Muxer::~Muxer()
{
    
}

static void releaseResource(AVFormatContext *in_fmt1,AVFormatContext *in_fmt2,AVFormatContext *ou_fmt3)
{
    avformat_close_input(&in_fmt1);
    avformat_close_input(&in_fmt2);
    if (ou_fmt3 != NULL) {
        avformat_free_context(ou_fmt3);
    }
}

/** 将一个mp3音频文件和一个MP4无音频视频文件合并为一个MP4文件
 */
void Muxer::doMuxer()
{
    string curFile(__FILE__);
    unsigned long pos = curFile.find("1-video_encode_decode");
    if (pos == string::npos) {
        LOGD("not find 1-video_encode_decode");
        return;
    }
    
    string srcDic = curFile.substr(0,pos) + "filesources/";
    string aduio_srcPath = srcDic + "test-mp3-1.mp3";
    string video_srcPath = srcDic + "test_1280x720_1.mp4";
    string dstPath = "test.MP4";
    
    AVFormatContext *audio_fmtCtx = NULL,*video_fmtCtx = NULL;
    AVFormatContext *out_fmtCtx = NULL;
    int audio_stream_index = -1;
    int video_stream_index = -1;
    AVStream *audio_stream = NULL,*video_stream = NULL;
    
    int ret = 0;
    // 解封装音频文件
    ret = avformat_open_input(&audio_fmtCtx,aduio_srcPath.c_str(),NULL,NULL);
    if (ret < 0) {
        LOGD("avformat_open_input audio fail %d",ret);
        return;
    }
    ret = avformat_find_stream_info(audio_fmtCtx,NULL);
    if (ret < 0) {
        LOGD("avformat_find_stream_info audio fail");
        releaseResource(audio_fmtCtx, NULL, NULL);
        return;
    }
    for (int i= 0;i<audio_fmtCtx->nb_streams;i++) {
        AVStream *stream = audio_fmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    
    // 解封装视频文件
    ret = avformat_open_input(&video_fmtCtx,video_srcPath.c_str(),NULL,NULL);
    if (ret < 0) {
        LOGD("avformat_open_input video fail %d",ret);
        releaseResource(audio_fmtCtx, video_fmtCtx, NULL);
        return;
    }
    ret = avformat_find_stream_info(video_fmtCtx,NULL);
    if (ret < 0) {
        LOGD("avformat_find_stream_info fail");
        releaseResource(audio_fmtCtx, video_fmtCtx, NULL);
        return;
    }
    for (int i=0;i<video_fmtCtx->nb_streams;i++) {
        AVStream *stream = video_fmtCtx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    
    // 创建封装上下文，用于封装到MP4文件中
    ret = avformat_alloc_output_context2(&out_fmtCtx,NULL,NULL,dstPath.c_str());
    if (ret < 0) {
        LOGD("avformat_alloc_output_context2 fail %d",ret);
        releaseResource(audio_fmtCtx, video_fmtCtx, NULL);
        return;
    }
    
    // 添加用于输出的流，并赋值编码相关参数
    if (audio_stream_index != -1) {
        audio_stream = avformat_new_stream(out_fmtCtx,NULL);
        AVStream *sStream = audio_fmtCtx->streams[audio_stream_index];
        ret = avcodec_parameters_copy(audio_stream->codecpar,sStream->codecpar);
    }
    if (video_stream_index != -1) {
        video_stream = avformat_new_stream(out_fmtCtx,NULL);
        AVStream *sStream = video_fmtCtx->streams[video_stream_index];
        if (avcodec_parameters_copy(video_stream->codecpar,sStream->codecpar) < 0) {
            LOGD("avcodec_parameters_copy null");
            releaseResource(audio_fmtCtx, video_fmtCtx, out_fmtCtx);
            return;
        }
    }
    
    // 创建AVFormatContext的AVIOContext(参数采用系统默认)，用于封装时缓冲区
    if (!(out_fmtCtx->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmtCtx->pb,dstPath.c_str(),AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open fail");
            return;
        }
    }
    
    // 写入头文件
    if ((ret = avformat_write_header(out_fmtCtx,NULL)) < 0){
        LOGD("avformat_write_header %d",ret);
        releaseResource(audio_fmtCtx, video_fmtCtx, out_fmtCtx);
        return;
    }
    
    AVPacket *aSPacket = av_packet_alloc();
    AVPacket *vSPacket = av_packet_alloc();
    bool audio_finish = false,video_finish = false;
    bool found_audio = false,found_video = false;
    do {
        if (!audio_finish && !found_audio && audio_stream_index != -1) {
            if (av_read_frame(audio_fmtCtx,aSPacket)< 0) {
                audio_finish = true;
                found_audio = false;
            }
            if (!audio_finish && aSPacket->stream_index != audio_stream_index) {
                continue;
            }
            found_audio = true;
        }
        
        if (!video_finish && !found_video && video_stream_index != -1) {
            if (av_read_frame(video_fmtCtx,vSPacket) < 0) {
                video_finish = true;
                found_video = false;
            }
            if (!video_finish && vSPacket->stream_index != video_stream_index) {
                continue;
            }
            found_video = true;
        }
        
        // 写入数据 视频在前
        if ((found_video && found_audio)) {
            if (av_compare_ts(aSPacket->pts,audio_fmtCtx->streams[audio_stream_index]->time_base,
                              vSPacket->pts,video_fmtCtx->streams[video_stream_index]->time_base) <= 0) {
                // 写入视频
                if ((ret = av_write_frame(out_fmtCtx,vSPacket)) < 0) {
                    LOGD("av_write_frame audio 1fail %d",ret);
                    releaseResource(audio_fmtCtx, video_fmtCtx, out_fmtCtx);
                    break;
                }
                found_video = false;
                av_packet_unref(vSPacket);
            } else {
                // 写入音频
                if ((ret = av_write_frame(out_fmtCtx,aSPacket)) < 0) {
                    LOGD("av_write_frame audio 1 fail %d",ret);
                    releaseResource(audio_fmtCtx, video_fmtCtx, out_fmtCtx);
                    break;
                }
                found_audio = false;
                av_packet_unref(aSPacket);
            }
        } else if (found_video && !found_audio){
            // 写入视频
            if ((ret = av_write_frame(out_fmtCtx,vSPacket)) < 0) {
                LOGD("av_write_frame video 2 fail %d",ret);
                releaseResource(audio_fmtCtx, video_fmtCtx, out_fmtCtx);
                break;
            }
            found_video = false;
            av_packet_unref(vSPacket);
        } else if (!found_video && found_audio) {
            // 写入音频
            if ((ret = av_write_frame(out_fmtCtx,aSPacket)) < 0) {
                LOGD("av_write_frame audio 2 fail %d",ret);
                break;
            }
            found_audio = false;
            av_packet_unref(aSPacket);
        }
        
    } while(!audio_finish || !video_finish);
    
    
    // 释放内存
    avformat_close_input(&audio_fmtCtx);
    avformat_close_input(&video_fmtCtx);
    if (out_fmtCtx) {
        avformat_free_context(out_fmtCtx);
    }
    
    
}
