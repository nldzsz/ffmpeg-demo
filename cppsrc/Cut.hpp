//
//  Cut.hpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/18.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef Cut_hpp
#define Cut_hpp

#include <stdio.h>
#include <string>
#include <iomanip>
#include <chrono>
#include "CLog.h"
#include <iostream>
extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}
using namespace std;

// 打开 .hpp line 33 .cpp line 100
class Cut
{
public:
    /** 实现时间的精准裁剪
     */
    void doCut(string spath,string dpath);
private:
    
    int64_t start_pos;
    int64_t video_start_pts;
    int64_t audio_start_pts;
    int64_t duration;           // 时长 单位秒
    int64_t video_next_pts;
    int64_t audio_next_pts;
    
    // 源文件
    string srcPath;
    // 目标文件
    string dstPath;
    // 视频流索引
    int video_in_stream_index,video_ou_tream_index;
    // 音频流索引
    int audio_in_stream_index,audio_ou_stream_index;
    bool has_writer_header;
    // 用于解封装
    AVFormatContext *in_fmtctx;
    // 用于封装
    AVFormatContext *ou_fmtctx;
    // 视频编码和解码用
    AVFrame *video_de_frame;
    AVFrame *video_en_frame;
    AVCodecContext *video_de_ctx;
    AVCodecContext *video_en_ctx;
    
    void doDecode(AVPacket *inpkt);
    void doEncode(AVFrame *enfram);
    void doWrite(AVPacket *pkt);
    void releasesources();
};

#endif /* Cut_hpp */
