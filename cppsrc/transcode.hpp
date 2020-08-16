//
//  transcode.h
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/3.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef transcode_h
#define transcode_h

#include <stdio.h>
#include <string>
#include <vector>

extern "C" {
#include "cppcommon/CLog.h"
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/channel_layout.h>
#include <libavutil/timestamp.h>
}

using namespace std;
// 今日打开，完工and 写博客
class Transcode
{
public:
    Transcode();
    ~Transcode();
    // 对封装容器进行转化，比如MP4,MOV,FLV,TS等容器之间的转换
    void doExtensionTranscode(string srcPath,string dstPath);
    // 做转码(包括编码方式，码率，分辨率，采样率，采样格式等等的转换)
    void doTranscode(string sPath,string dPath);
private:
    bool openFile();
    bool add_video_stream();
    bool add_audio_stream();
    void doDecodeVideo(AVPacket *packet);
    void doEncodeVideo(AVFrame *frame);
    void doDecodeAudio(AVPacket *packet);
    void doEncodeAudio(AVFrame *frame);
    void doWrite(AVPacket* packet,bool isVideo);
    void releaseSources();
    
    int video_in_stream_index;
    int video_ou_stream_index;
    int audio_in_stream_index;
    int audio_ou_stream_index;
    int64_t video_pts;
    int64_t audio_pts;
    
    AVFormatContext *inFmtCtx;
    AVFormatContext *ouFmtCtx;
    uint8_t         **audio_buffer;
    int             left_size;
    bool            audio_init;
    
    // 用于音频解码
    AVCodecContext  *audio_de_ctx;
    // 用于视频解码
    AVCodecContext  *video_de_ctx;
    // 用于音频编码
    AVCodecContext  *audio_en_ctx;
    // 用于视频编码
    AVCodecContext  *video_en_ctx;
    // 用于音频格式转换
    SwrContext      *swr_ctx;
    // 用于视频格式转换
    SwsContext      *sws_ctx;
    bool            audio_need_convert;
    bool            video_need_convert;
    AVFrame         *video_de_frame;
    AVFrame         *audio_de_frame;
    AVFrame         *video_en_frame;
    AVFrame         *audio_en_frame;
    AVCodec         *video_en_codec;
    
    string          srcPath;
    string          dstPath;
    
    // 用于转码用的参数结构体
    typedef struct TranscodePar{
        bool copy;  // 是否和源文件保持一致
        union {
            enum AVCodecID codecId;
            enum AVPixelFormat pix_fmt;
            enum AVSampleFormat smp_fmt;
            int64_t i64_val;
            int     i32_val;
        } par_val;
    }TranscodePar;
#define Par_CodeId(copy,val) {copy,{.codecId=val}}
#define Par_PixFmt(copy,val) {copy,{.pix_fmt=val}}
#define Par_SmpFmt(copy,val) {copy,{.smp_fmt=val}}
#define Par_Int64t(copy,val) {copy,{.i64_val=val}}
#define Par_Int32t(copy,val) {copy,{.i32_val=val}}
    
    enum AVCodecID  src_video_id;
    enum AVCodecID  src_audio_id;
    TranscodePar    dst_video_id;
    TranscodePar    dst_audio_id;
    TranscodePar    dst_channel_layout;
    TranscodePar    dst_video_bit_rate;
    TranscodePar    dst_audio_bit_rate;
    TranscodePar    dst_sample_rate;
    TranscodePar    dst_width;
    TranscodePar    dst_height;
    TranscodePar    dst_fps;
    TranscodePar    dst_pix_fmt;
    TranscodePar    dst_sample_fmt;
    bool            video_need_transcode;
    bool            audio_need_transcode;
    
    vector<AVPacket *> videoCache;
    vector<AVPacket *> audioCache;
    int64_t last_video_pts;
    int64_t last_audio_pts;
    
    AVFrame *get_video_frame(enum AVPixelFormat pixfmt,int width,int height);
    AVFrame *get_audio_frame(enum AVSampleFormat smpfmt,int64_t ch_layout,int sample_rate,int nb_samples);
};

#endif /* transcode_h */
