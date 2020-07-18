//
//  audio_acrossfade.hpp
//  demo-mac
//
//  Created by apple on 2020/7/15.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef audio_acrossfade_hpp
#define audio_acrossfade_hpp

#include <stdio.h>
#include <string>
using namespace std;
extern "C"
{
#include "Common.h"
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/timestamp.h>
}
class AudioAcrossfade
{
public:
    AudioAcrossfade();
    virtual ~AudioAcrossfade();
    
    /** 实现两个输入音频的拼接，并且实现第一个音频的结尾和第二个音频的开头混合，混合的效果为淡入淡出的效果
     *  备注：这里只考虑滤镜的使用方式，所以简单化处理，假设两个音频文件的编码方式，封装格式都一样。
     *  apath1:要拼接的第一个音频路径
     *  apath2:要拼接的第二个音频路径
     *  dpath:拼接后保存路径
     *  duration:两个音频文件混音的时长，单位秒，默认5秒
     *
     */
    void doAcrossfade(string apath1,string apath2,string dpath,int duration=5);
    
private:
    AVFormatContext *infmt1;
    AVFormatContext *infmt2;
    AVCodecContext  *de_ctx1;
    AVCodecContext  *de_ctx2;
    AVFrame         *de_frame1;
    AVFrame         *de_frame2;
    AVCodecContext  *en_ctx;
    AVFrame         *en_frame;
    AVFormatContext *oufmt;
    AVFilterGraph   *filterGraph;
    AVFilterInOut   *inputs;
    AVFilterInOut   *ouputs;
    AVFilterContext *source_filter_ctx;
    AVFilterContext *sink_filter_ctx;
    int             cur_index;
  
    bool openStream(AVFormatContext**infmt,AVCodecContext**de_ctx,string path);
    void doDecodec(AVPacket *pkt,int stream_index);
    void doEncodec(AVFrame  *frame);
    void doWrite(AVPacket *pkt);
    bool initFilterGraph(int duration);
    void internalRelease();
};

#endif /* audio_acrossfade_hpp */
