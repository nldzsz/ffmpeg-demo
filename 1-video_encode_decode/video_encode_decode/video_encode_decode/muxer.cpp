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

/** 遇到问题：avformat_close_input奔溃
 *  分析原因：最开始是这样定义releaseResource(AVFormatContext *in_fmt1,AVFormatContext *in_fmt2,AVFormatContext *ou_fmt3)函数的；in_fmt1是一个指针变量，当外部调用此函数时
 *  如果传递的实参如果是一个临时变量，那么在此函数内部就算在avformat_close_input(in_fmt1);后面执行in_fmt1=NULL;它也不能将外部传进来的实参置为NULL，如果多次调用
 *  releaseResource()函数并且传递的实参in_fmt1还是同一个，就会造成avformat_close_input释放多次，所以造成奔溃。
 *  解决方案：将函数定义成如下方式static void releaseResource(AVFormatContext **in_fmt1,AVFormatContext **in_fmt2,AVFormatContext **ou_fmt3)；事实上ffmpeg的很多释放函数也
 *  是采用的指针的指针作为形参的定义方式。
 */
static void releaseResource(AVFormatContext **in_fmt1,AVFormatContext **in_fmt2,AVFormatContext **ou_fmt3)
{
    if (in_fmt1 && *in_fmt1) {
        avformat_close_input(in_fmt1);
        *in_fmt1 = NULL;
    }
    if (in_fmt2 && *in_fmt2) {
        avformat_close_input(in_fmt2);
        *in_fmt2 = NULL;
    }
    if (ou_fmt3 && *ou_fmt3 != NULL) {
        avformat_free_context(*ou_fmt3);
        *ou_fmt3 = NULL;
    }
}

void Muxer::doReMuxer()
{
    string curFile(__FILE__);
    unsigned long pos = curFile.find("1-video_encode_decode");
    if (pos == string::npos) {
        LOGD("can not find file");
        return;
    }
    string srcDir = curFile.substr(0,pos) + "filesources/";
    string srcPath = srcDir + "test_1280x720_1.mp4";
    string dstPath = "1-test_1280_720_1.MP4";
    
    AVFormatContext *in_fmtCtx = NULL, *ou_fmtCtx = NULL;
    AVStream *ou_audio_stream = NULL,*ou_video_stream = NULL;
    int in_audio_index = -1,in_video_index = -1;
    int ret = 0;
    if ((ret = avformat_open_input(&in_fmtCtx,srcPath.c_str(),NULL,NULL)) < 0) {
        LOGD("avformat_open_input fail %d",ret);
        return;
    }
    if (avformat_find_stream_info(in_fmtCtx ,NULL) < 0) {
        LOGD("avformat_find_stream_info() fail");
        releaseResource(&in_fmtCtx, NULL, NULL);
        return;
    }
    for(int i=0;i<in_fmtCtx->nb_streams;i++) {
        AVStream *stream = in_fmtCtx->streams[i];
        if (in_audio_index == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            in_audio_index = i;
        }
        if (in_video_index == -1 && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            in_video_index = i;
        }
    }
    
    // 创建上下文
    if (avformat_alloc_output_context2(&ou_fmtCtx,NULL,NULL,dstPath.c_str()) < 0){
        LOGD("avformat_alloc_output_context2 fail");
        releaseResource(&in_fmtCtx, NULL, NULL);
        return;
    }
    
    // 添加流
    if (in_audio_index != -1) {
        ou_audio_stream = avformat_new_stream(ou_fmtCtx,NULL);
        avcodec_parameters_copy(ou_audio_stream->codecpar,in_fmtCtx->streams[in_audio_index]->codecpar);
    }
    if (in_video_index != -1) {
        ou_video_stream = avformat_new_stream(ou_fmtCtx,NULL);
        avcodec_parameters_copy(ou_video_stream->codecpar,in_fmtCtx->streams[in_video_index]->codecpar);
    }
    
    // 打开AVIOContext缓冲区
    if (!(ou_fmtCtx->flags & AVFMT_NOFILE)) {
        if (avio_open(&ou_fmtCtx->pb,dstPath.c_str(),AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open fail");
            releaseResource(&in_fmtCtx, NULL, &ou_fmtCtx);
            return;
        }
    }
    
    // 写入头部信息
    if (avformat_write_header(ou_fmtCtx, NULL) < 0) {
        LOGD("avformat_write_header fail");
        return;
    }
    
    AVPacket *sPacket = av_packet_alloc();
    while (av_read_frame(in_fmtCtx, sPacket) >= 0) {
        LOGD("pts %d dts %d index %d",sPacket->pts,sPacket->dts,sPacket->stream_index);
        if (sPacket->stream_index == in_audio_index) {
            AVStream *stream = in_fmtCtx->streams[in_audio_index];
            sPacket->pts = av_rescale_q_rnd(sPacket->pts,stream->time_base,ou_audio_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->dts = av_rescale_q_rnd(sPacket->dts,stream->time_base,ou_audio_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->duration = av_rescale_q_rnd(sPacket->duration,stream->time_base,ou_audio_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->stream_index = ou_audio_stream->index;
            if (av_write_frame(ou_fmtCtx, sPacket) < 0) {
                LOGD("av_write_frame 1 fail ");
                break;
            }
        } else if (sPacket->stream_index == in_video_index) {
            AVStream *stream = in_fmtCtx->streams[in_video_index];
            sPacket->pts = av_rescale_q_rnd(sPacket->pts,stream->time_base,ou_video_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->dts = av_rescale_q_rnd(sPacket->dts,stream->time_base,ou_video_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->duration = av_rescale_q_rnd(sPacket->duration,stream->time_base,ou_video_stream->time_base,AV_ROUND_NEAR_INF);
            sPacket->stream_index = ou_video_stream->index;
            if (av_write_frame(ou_fmtCtx, sPacket) < 0) {
                LOGD("av_write_frame 2 fial");
                break;
            }
        }
        
        av_packet_unref(sPacket);
    }
    
    LOGD("写入完毕");
    // 写入尾部信息
    ret = av_write_trailer(ou_fmtCtx);
    releaseResource(&in_fmtCtx, NULL, &ou_fmtCtx);
    
}

/** 将一个mp3音频文件和一个MP4无音频视频文件合并为一个MP4文件
 */
void Muxer::doMuxerTwoFile()
{
    string curFile(__FILE__);
    unsigned long pos = curFile.find("1-video_encode_decode");
    if (pos == string::npos) {
        LOGD("not find 1-video_encode_decode");
        return;
    }
    
    string srcDic = curFile.substr(0,pos) + "filesources/";
    /** 遇到问题：最终合成的MP4文件 在苹果系产品(mac iOS下默认播放器无声音)，但是VLC及ffplay播放正常。在安卓/windows下默认播放器正常
     */
    string aduio_srcPath = srcDic + "test-mp3-1.mp3";
//    string aduio_srcPath = srcDic + "test_441_f32le_2.aac";
    string video_srcPath = srcDic + "test_1280x720_2.mp4";
    string dstPath = "test.MP4";
    
    AVFormatContext *audio_fmtCtx = NULL,*video_fmtCtx = NULL;
    AVFormatContext *out_fmtCtx = NULL;
    int audio_stream_index = -1;
    int video_stream_index = -1;
    AVStream *audio_in_stream = NULL,*video_in_stream = NULL;
    AVStream *audio_ou_stream = NULL,*video_ou_stream = NULL;
    
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
        releaseResource(&audio_fmtCtx, NULL, NULL);
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
        releaseResource(&audio_fmtCtx, &video_fmtCtx, NULL);
        return;
    }
    /** 遇到问题：当输入音频文件是mp3时，执行avformat_find_stream_info()函数时提示"Could not find codec parameters for stream 0 (Audio: mp3, 44100 Hz, 2 channels, 128 kb/s):
     *  unspecified frame sizeConsider increasing the value for the 'analyzeduration' and 'probesize' options";
     *  分析原因：库中没有编译mp3解码器导致avformat_find_stream_info()无法解码从而出现这样的错误
     *  解决方案：重新编译ffmpeg带上mp3解码器
     *  备注：avformat_find_stream_info()内部函数默认会尝试进行读取probesize的数据进行解码工作，所以对应的解码器必须要编译进来，否则此函数执行会出现上面错误
     */
    ret = avformat_find_stream_info(video_fmtCtx,NULL);
    if (ret < 0) {
        LOGD("avformat_find_stream_info fail");
        releaseResource(&audio_fmtCtx, &video_fmtCtx, NULL);
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
        releaseResource(&audio_fmtCtx, &video_fmtCtx, NULL);
        return;
    }
    
    // 添加用于输出的流，并赋值编码相关参数
    if (audio_stream_index != -1) {
        audio_ou_stream = avformat_new_stream(out_fmtCtx,NULL);
        audio_in_stream = audio_fmtCtx->streams[audio_stream_index];
        ret = avcodec_parameters_copy(audio_ou_stream->codecpar,audio_in_stream->codecpar);
    }
    if (video_stream_index != -1) {
        video_ou_stream = avformat_new_stream(out_fmtCtx,NULL);
        video_in_stream = video_fmtCtx->streams[video_stream_index];
        if (avcodec_parameters_copy(video_ou_stream->codecpar,video_in_stream->codecpar) < 0) {
            LOGD("avcodec_parameters_copy null");
            releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
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
        releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
        return;
    }
    
    AVPacket *aSPacket = av_packet_alloc();
    AVPacket *vSPacket = av_packet_alloc();
    bool audio_finish = false,video_finish = false;
    bool found_audio = false,found_video = false;
    int64_t org_pts = 0;
    do {
        if (!audio_finish && !found_audio && audio_stream_index != -1) {
            if (av_read_frame(audio_fmtCtx,aSPacket)< 0) {
                audio_finish = true;
            }
            // 不是所需的音频数据
            if (!audio_finish && /**found_audio && */aSPacket->stream_index != audio_stream_index) {
                av_packet_unref(aSPacket);
                continue;
            }
            
            if (!audio_finish) {
                found_audio = true;
                // 改变packet的pts,dts,duration
                aSPacket->stream_index = audio_ou_stream->index;
                aSPacket->pts = av_rescale_q_rnd(org_pts,audio_in_stream->time_base,audio_ou_stream->time_base,AV_ROUND_UP);
                aSPacket->dts = av_rescale_q_rnd(org_pts,audio_in_stream->time_base,audio_ou_stream->time_base,AV_ROUND_UP);
                org_pts += aSPacket->duration;
                aSPacket->duration = av_rescale_q_rnd(aSPacket->duration,audio_in_stream->time_base,audio_ou_stream->time_base,AV_ROUND_UP);
                aSPacket->pos = -1;
                // 要写入的packet的stream_index必须要设置正确
                aSPacket->stream_index = audio_ou_stream->index;
                
                AVRational tb = audio_ou_stream->time_base;
                LOGD("audio pts:%s dts:%s duration %s size %d finish %d",
                     av_ts2timestr(aSPacket->pts,&tb),
                     av_ts2timestr(aSPacket->dts,&tb),
                     av_ts2timestr(aSPacket->duration,&tb),
                     aSPacket->size,audio_finish);
            }
        }
        
        if (!video_finish && !found_video && video_stream_index != -1) {
            if (av_read_frame(video_fmtCtx,vSPacket) < 0) {
                video_finish = true;
            }
            
            /** 遇到问题：写入视频数据时提示"[mp4 @ 0x10100ae00] Application provided invalid, non monotonically increasing dts to muxer in stream 1: 8000 >= 0"错误
             *  分析原因：调用av_write_frame()写入视频或者音频时 dts必须是依次增长的(pts 可以不用)，这里是因为这里有个逻辑错误，将视频文件中的音频数据误当做视频
             *  来写入了，导致了dts的错误;如下，多加了一个found_video变量
             */
            if (!video_finish && /**found_video && */vSPacket->stream_index != video_stream_index) {
                av_packet_unref(vSPacket);
                continue;
            }
            
            if (!video_finish) {
                found_video = true;
                /** 遇到问题：写入视频的帧率信息
                 *  分析原因：avformat_parameters_copy()只是将编码参数进行了赋值，再进行封装时，帧率，视频时长是根据AVPacket的pts,dts,duration进行
                 *  计算的，所以这个时间就一定要和AVstream的time_base对应。
                 *  解决方案：进行如下的时间基的转换
                 */
                // 要写入的packet的stream_index必须要设置正确
                vSPacket->stream_index = video_ou_stream->index;
                vSPacket->pts = av_rescale_q_rnd(vSPacket->pts,video_in_stream->time_base,video_ou_stream->time_base,AV_ROUND_INF);
                vSPacket->dts = av_rescale_q_rnd(vSPacket->dts, video_in_stream->time_base, video_ou_stream->time_base, AV_ROUND_INF);
                vSPacket->duration = av_rescale_q_rnd(vSPacket->duration, video_in_stream->time_base, video_ou_stream->time_base, AV_ROUND_INF);
                
                AVRational tb = video_in_stream->time_base;
                LOGD("video pts:%s dts:%s duration %s size %d key:%d finish %d",
                     av_ts2timestr(vSPacket->pts,&tb),
                     av_ts2timestr(vSPacket->dts,&tb),
                     av_ts2timestr(vSPacket->duration,&tb),
                     vSPacket->size,vSPacket->flags&AV_PKT_FLAG_KEY,video_finish);
            }
        }
        
        // 打印日志
//        if (found_audio && !audio_finish) {
//            AVRational tb = audio_in_stream->time_base;
//            LOGD("audio pts:%s dts:%s duration %s size %d finish %d",
//                 av_ts2timestr(aSPacket->pts,&tb),
//                 av_ts2timestr(aSPacket->dts,&tb),
//                 av_ts2timestr(aSPacket->duration,&tb),
//                 aSPacket->size,audio_finish);
//        }
//        if (found_video && !video_finish) {
//            AVRational tb = video_in_stream->time_base;
//            LOGD("video pts:%s dts:%s duration %s size %d key:%d finish %d",
//                 av_ts2timestr(vSPacket->pts,&tb),
//                 av_ts2timestr(vSPacket->dts,&tb),
//                 av_ts2timestr(vSPacket->duration,&tb),
//                 vSPacket->size,vSPacket->flags&AV_PKT_FLAG_KEY,video_finish);
//        }
        
        /** 遇到问题：[mp4 @ 0x10200b400] Timestamps are unset in a packet for stream 0. This is deprecated and will stop working in the future.
         *  Fix your code to set the timestamps properly
         *  分析原因：从mp3读取的packet，其pts和dts均为0
         */
        // 写入数据 视频在前
        if (found_video && found_audio && !video_finish && !audio_finish) {
            if ((ret = av_compare_ts(aSPacket->pts,audio_ou_stream->time_base,
                              vSPacket->pts,video_ou_stream->time_base)) > 0) {
                // 写入视频
                if ((ret = av_write_frame(out_fmtCtx,vSPacket)) < 0) {
                    LOGD("av_write_frame video 1 fail %d",ret);
                    releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
                    break;
                }
                LOGD("写入了视频 1");
                found_video = false;
                av_packet_unref(vSPacket);
            } else {
                // 写入音频
                if ((ret = av_write_frame(out_fmtCtx,aSPacket)) < 0) {
                    LOGD("av_write_frame audio 1 fail %d",ret);
                    releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
                    break;
                }
                LOGD("写入了音频 1");
                found_audio = false;
                av_packet_unref(aSPacket);
            }
        } else if (!video_finish && found_video && !found_audio){
            // 写入视频
            if ((ret = av_write_frame(out_fmtCtx,vSPacket)) < 0) {
                LOGD("av_write_frame video 2 fail %d",ret);
                releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
                break;
            }
            LOGD("写入了视频 2");
            found_video = false;
            av_packet_unref(vSPacket);
        } else if (!found_video && found_audio && !audio_finish) {
            // 写入音频
            if ((ret = av_write_frame(out_fmtCtx,aSPacket)) < 0) {
                LOGD("av_write_frame audio 2 fail %d",ret);
                break;
            }
            LOGD("写入了音频 2");
            found_audio = false;
            av_packet_unref(aSPacket);
        }
        
    } while(!audio_finish && !video_finish);
    
    // 结束写入
    av_write_trailer(out_fmtCtx);
    
    // 释放内存
    releaseResource(&audio_fmtCtx, &video_fmtCtx, &out_fmtCtx);
}
