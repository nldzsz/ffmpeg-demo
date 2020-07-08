//
//  VideoScale.hpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/6/10.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef VideoScale_hpp
#define VideoScale_hpp

#include <stdio.h>
#include <string>
extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/timestamp.h>
#include "CLog.h"
}
using namespace std;

class FilterVideoScale
{
public:
    FilterVideoScale();
    ~FilterVideoScale();
    /** 功能：利用滤镜实现对视频的压缩和翻转
     *  1、只是对视频进行处理，故目标文件的格式和源文件保持一致
     */
    void doVideoScale(string srcpath,string dstpath);
private:
    AVCodecContext  *en_ctx;
    AVCodecContext  *de_ctx;
    AVFormatContext *in_fmt;
    AVFormatContext *ou_fmt;
    AVFilterInOut   *inputs;
    AVFilterInOut   *ouputs;
    AVFilterContext *src_filter_ctx;
    AVFilterContext *sink_filter_ctx;
    AVFilterGraph   *graph;
    AVFrame         *de_frame;
    AVFrame         *en_frame;
    int             video_in_index;
    int             audio_in_index;
    int             video_ou_index;
    int             audio_ou_index;
    int             dst_width;
    int             dst_height;
    
    void internalrelease();
    
    // 初始化视频滤镜
    bool init_vidoo_filter_graph();
    void doVideoDecodec(AVPacket *pkt);
    void doVideoEncode(AVFrame *frame);
};

#endif /* VideoScale_hpp */
