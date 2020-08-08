//
//  Subtitles.hpp
//  demo-mac
//
//  Created by apple on 2020/7/22.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef Subtitles_hpp
#define Subtitles_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
using namespace::std;

extern "C"
{
#include "Common.h"
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/timestamp.h>
}
class Subtitles
{
public:
    Subtitles();
    ~Subtitles();
    
    /** 实现功能：为视频格式添加字幕流，这里字幕流的格式为srt/ass/vtt等等。这里简化需求，假设源视频文件的音视频编码方式也被mkv支持
     *
     *  1、不需要重新编解码，速度比较快。字幕流作为和音视频流同等地位存在于文件容器中
     *  2、包含字幕流的文件要想字幕被正确渲染出来需要播放器的支持，而且不同的播放器可能对字幕渲染的实现
     *      方式不一样会导致最后看到的字幕呈现方式也会不一样
     */
    void addSubtitleStream(string videopath,string spath,string dstpath);
    
    /** 实现功能：将字幕流嵌入到视频中
     *  1、要重新编解码，速度比较慢，字幕流最终被嵌入到了视频帧中
     *  2、由于字幕流已经被嵌入到了视频中，作为视频画面的一部分存在所以不同的播放器最终呈现的字幕效果一致。
     */
    bool configConfpath(string confpath,string fontsPath,map<string,string>fontmaps);
    void addSubtitlesForVideo(string vpath,string spath,string dstpath,string confpath);
    
    
private:
    AVFormatContext *vfmt;
    AVFormatContext *sfmt;
    AVFormatContext *ofmt;
    
    int in_video_index,in_audio_index;
    int ou_video_index,ou_audio_index;
    AVCodecContext  *de_video_ctx;
    AVCodecContext  *en_video_ctx;
    AVFrame         *de_frame;
    AVFilterGraph   *graph;
    AVFilterContext *src_filter_ctx;
    AVFilterContext *sink_filter_ctx;
    
    void doDecodec(AVPacket *pkt);
    void doEncodec(AVFrame *frame);
    bool initFilterGraph(string spath,string confpath);
    void releaseInternal();
};

#endif /* Subtitles_hpp */
