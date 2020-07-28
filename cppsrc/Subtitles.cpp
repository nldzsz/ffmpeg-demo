//
//  Subtitles.cpp
//  demo-mac
//
//  Created by apple on 2020/7/22.
//  Copyright © 2020 apple. All rights reserved.
//

#include "Subtitles.hpp"

Subtitles::Subtitles()
{
    vfmt = NULL;
    sfmt = NULL;
    ofmt = NULL;
}

Subtitles::~Subtitles()
{
    
}

void Subtitles::releaseInternal()
{
    
}

/** 对于mkv的封装和解封装要开启ffmpeg的编译参数 --enable-muxer=matroska和--enable-demuxer=matroska
 *
 *  备注：不同格式的字幕ass/srt写入文件后，当用播放器打开的时候字幕的大小以及位置也有区别
 */
void Subtitles::addSubtitleStream(string videopath, string spath, string dstpath)
{
    if (dstpath.rfind(".mkv") != dstpath.length() - 4) {
        LOGD("can only suport .mkv file");
        return;
    }
    
    int ret = 0;
    // 打开视频流
    if (avformat_open_input(&vfmt,videopath.c_str(), NULL, NULL) < 0) {
        LOGD("avformat_open_input failed");
        return;
    }
    if (avformat_find_stream_info(vfmt, NULL) < 0) {
        LOGD("avformat_find_stream_info");
        releaseInternal();
        return;
    }
    
    if ((avformat_alloc_output_context2(&ofmt, NULL, NULL, dstpath.c_str())) < 0) {
        LOGD("avformat_alloc_output_context2() failed");
        releaseInternal();
        return;
    }
    
    int in_video_index = -1,in_audio_index = -1;
    int ou_video_index = -1,ou_audio_index = -1,ou_subtitle_index = -1;
    for (int i=0; i<vfmt->nb_streams; i++) {
        AVStream *stream = vfmt->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            in_video_index = i;
            AVStream *newstream = avformat_new_stream(ofmt, NULL);
            avcodec_parameters_copy(newstream->codecpar, stream->codecpar);
            newstream->codecpar->codec_tag = 0;
            ou_video_index = newstream->index;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVStream *newstream = avformat_new_stream(ofmt, NULL);
            avcodec_parameters_copy(newstream->codecpar, stream->codecpar);
            newstream->codecpar->codec_tag = 0;
            in_audio_index = i;
            ou_audio_index = newstream->index;
        }
    }
    if (!(ofmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open failed");
            releaseInternal();
            return;
        }
    }
    
    // 打开字幕流
    /** 遇到问题：调用avformat_open_input()时提示"avformat_open_input failed -1094995529(Invalid data found when processing input)"
     *  分析原因：编译ffmpeg库是没有将对应的字幕解析器添加进去比如(ff_ass_demuxer,ff_ass_muxer)
     *  解决方案：添加对应的编译参数
     */
    if ((ret = avformat_open_input(&sfmt,spath.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input failed %d(%s)",ret,av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(sfmt, NULL)) < 0) {
        LOGD("avformat_find_stream_info %d(%s)",ret,av_err2str(ret));
        releaseInternal();
        return;
    }
    
    if((ret = av_find_best_stream(sfmt, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0))<0){
        LOGD("not find subtitle stream 0");
        releaseInternal();
        return;
    }
    AVStream *nstream = avformat_new_stream(ofmt, NULL);
    ret = avcodec_parameters_copy(nstream->codecpar, sfmt->streams[0]->codecpar);
    nstream->codecpar->codec_tag = 0;
    /** todo:zsz AV_DISPOSITION_xxx：ffmpeg.c中该选项可以控制字幕默认是否显示，不过这里貌似不可以，原因未知。
     */
//    nstream->disposition = sfmt->streams[0]->disposition;
    ou_subtitle_index = nstream->index;
    
    if(avformat_write_header(ofmt, NULL)<0){
        LOGD("avformat_write_header failed");
        releaseInternal();
        return;
    }
    av_dump_format(ofmt, 0, dstpath.c_str(), 1);
    
    /** 遇到问题：封装后生成的mkv文件字幕无法显示，封装时提示"[matroska @ 0x10381c000] Starting new cluster due to timestamp"
     *  分析原因：通过和ffmpeg.c中源码进行比对，后发现mvk对字幕写入的顺序有要求
     *  解决方案：将字幕写入放到音视频之前
     */
    AVPacket *inpkt2 = av_packet_alloc();
    while (av_read_frame(sfmt, inpkt2) >= 0) {
        
        AVStream *srcstream = sfmt->streams[0];
        AVStream *dststream = ofmt->streams[ou_subtitle_index];
        av_packet_rescale_ts(inpkt2, srcstream->time_base, dststream->time_base);
        inpkt2->stream_index = ou_subtitle_index;
        inpkt2->pos = -1;
        LOGD("pts %d",inpkt2->pts);
        if (av_write_frame(ofmt, inpkt2) < 0) {
            LOGD("subtitle av_write_frame failed");
            releaseInternal();
            return;
        }
        av_packet_unref(inpkt2);
    }
    
    AVPacket *inpkt = av_packet_alloc();
    while (av_read_frame(vfmt, inpkt) >= 0) {
        
        if (inpkt->stream_index == in_video_index) {
            AVStream *srcstream = vfmt->streams[in_video_index];
            AVStream *dststream = ofmt->streams[ou_video_index];
            av_packet_rescale_ts(inpkt, srcstream->time_base, dststream->time_base);
            inpkt->stream_index = ou_video_index;
            LOGD("inpkt %d",inpkt->pts);
            if (av_write_frame(ofmt, inpkt) < 0) {
                LOGD("video av_write_frame failed");
                releaseInternal();
                return;
            }
        } else if (inpkt->stream_index == in_audio_index) {
            AVStream *srcstream = vfmt->streams[in_audio_index];
            AVStream *dststream = ofmt->streams[ou_audio_index];
            av_packet_rescale_ts(inpkt, srcstream->time_base, dststream->time_base);
            inpkt->stream_index = ou_audio_index;
            if (av_write_frame(ofmt, inpkt) < 0) {
                LOGD("audio av_write_frame failed");
                releaseInternal();
                return;
            }
        }
        
        av_packet_unref(inpkt);
    }
    
    LOGD("over");
    av_write_trailer(ofmt);
    releaseInternal();
    
}

/** ffmpeg中字幕处理的滤镜有两个subtitles和drawtext。
 *  1、要想正确使用subtitles滤镜，编译ffmpeg时需要添加--enable-libass --enable-filter=subtitles配置参数，同时引入libass库。同时由于libass库又引用了freetype,fribidi外部库所以还需要同时编译这两个库，此外
 *  libass库根据操作系统的不同还引入不同的外部库，比如mac os系统则引入了CoreText.framework库,Linux则引入了fontconfig库，windows系统则引入了DirectWrite，或者添加--disable-require-system-font-provider
 *  代表不使用这些系统的库
 *  2、要想正确使用drawtext滤镜，编译ffmpeg时需要添加--enable-filter=drawtext同时要引入freetype和fribidi外部库
 *  3、所以libass和drawtext滤镜从本质上看都是调用freetype生成一张图片，然后再将图片和视频融合
 *
 *  与libass库字幕处理相关的三个库：
 *  1、text shaper相关：用来定义字体形状相关，fribidi和HarfBuzz两个库，其中fribidi速度较快，与字体库形状无关的一个库，libass默认，故HarfBuzz可以选择不编译
 *  2、字体库相关：CoreText(ios/mac)；fontconfig(linux/android/ios/mac);DirectWrite(windows)，用来创建字体。
 *  3、freetype：用于将字符串按照前面指定的字体以及字体形状渲染为字体图像(RGB格式，备注：它还可以将RGB格式最终输出为PNG，则需要编译libpng库)
 *
 *  所以libass则是对上面三个库的封装，用以更加方便的实现为一帧视频添加字幕图像的功能
 */
void Subtitles::addSubtitlesForVideo(string vpath, string spath, string dstpath)
{
    int ret = 0;
    // 打开视频流
    if (avformat_open_input(&vfmt,vpath.c_str(), NULL, NULL) < 0) {
        LOGD("avformat_open_input failed");
        return;
    }
    if (avformat_find_stream_info(vfmt, NULL) < 0) {
        LOGD("avformat_find_stream_info");
        releaseInternal();
        return;
    }
    
    // 打开字幕流
    /** 遇到问题：调用avformat_open_input()时提示"avformat_open_input failed -1094995529(Invalid data found when processing input)"
     *  分析原因：编译ffmpeg库是没有将对应的字幕解析器添加进去比如(ff_ass_demuxer,ff_ass_muxer)
     *  解决方案：添加对应的编译参数
     */
    if ((ret = avformat_open_input(&sfmt,spath.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input failed %d(%s)",ret,av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(sfmt, NULL)) < 0) {
        LOGD("avformat_find_stream_info %d(%s)",ret,av_err2str(ret));
        releaseInternal();
        return;
    }
    
    
}

