//
//  VideoJpg.hpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/20.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef VideoJpg_hpp
#define VideoJpg_hpp

#include <stdio.h>
#include <string>

extern "C" {
#include "cppcommon/CLog.h"
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}
using namespace std;

class VideoJPG
{
public:
    VideoJPG();
    ~VideoJPG();
    
    /** 功能：实现提取任意时刻视频的某一帧并将其转化为JPG图片输出到当前目录
     */
    void doJpgGet();
    
    
    /** 功能：将多张JPG照片合并成一段视频
     */
    void doJpgToVideo();
private:
    AVFrame *de_frame;
    AVFrame *en_frame;
    // 用于视频像素转换
    SwsContext *sws_ctx;
    // 用于读取视频
    AVFormatContext *in_fmt;
    // 用于解码
    AVCodecContext *de_ctx;
    // 用于编码
    AVCodecContext *en_ctx;
    // 用于封装jpg
    AVFormatContext *ou_fmt;
    int video_ou_index;
    
    void releaseSources();
    void doDecode(AVPacket *in_pkt);
    void doEncode(AVFrame *en_frame);
    
};
#endif /* VideoJpg_hpp */
