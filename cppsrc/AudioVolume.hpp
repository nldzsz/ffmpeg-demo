//
//  AudioVolume.hpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/30.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef AudioVolume_hpp
#define AudioVolume_hpp

#include <stdio.h>
#include <inttypes.h>
#include <string>

using namespace std;
extern "C" {
#include "Common.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/opt.h>
#include <libavutil/macros.h>
#include <libavutil/timestamp.h>
}

class AudioVolume
{
public:
    AudioVolume();
    ~AudioVolume();
    /** 方式一：实现调整声音的大小以及改变音频数据的格式功能
     *  1、可以调整声音的大小
     *  2、可以改变音频的采样率，采样格式，声道类型等等格式
     *  使用音频滤镜实现，改变后达到想要的效果
     *  备注：以MP3音频为例；
     */
    void doChangeAudioVolume(string srcpath,string dstpath);
    
    /** 方式二：实现调整声音的大小以及改变音频数据的格式功能
     *  1、可以调整声音的大小
     *  2、可以改变音频的采样率，采样格式，声道类型等等格式
     *  使用音频滤镜实现，改变后达到想要的效果
     *  备注：以MP3音频为例；与方式一不同的地方在于滤镜管道的初始化方式更加简单
     */
    void doChangeAudioVolume2(string srcpath,string dstpath);
    
private:
    // 用于解析本地音频源文件
    AVFormatContext *in_fmt;
    // 用于写入改变后的音频数据到目标文件
    AVFormatContext *ou_fmt;
    // 用于解码的
    AVCodecContext  *de_ctx;
    // 用于编码的
    AVCodecContext  *en_ctx;
    AVFrame         *de_frame;
    AVFrame         *en_frame;
    // 处理音频转换的滤镜管道
    AVFilterGraph *graph;
    // 输入滤镜上下文(滤镜实例)
    AVFilterContext *src_flt_ctx;
    // 输出滤镜上下文(滤镜实例)
    AVFilterContext *sink_flt_ctx;
    // 输入滤镜
    AVFilter        *src_flt;
    // 输出滤镜
    AVFilter        *sink_flt;
    int             next_audio_pts;
    
    void doDecode(AVPacket *packt);
    void doEncode(AVFrame *frame);
    void releasesources();
};
#endif /* AudioVolume_hpp */
