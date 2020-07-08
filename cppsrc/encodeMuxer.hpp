//
//  encodeMuxer.hpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/10.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef encodeMuxer_hpp
#define encodeMuxer_hpp

#include <stdio.h>
#include <string>
#include "CLog.h"
extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
using namespace std;

class EncodeMuxer
{
public:
    void doEncodeMuxer(string dstPath);
    AVStream *video_ou_stream;
    AVCodecContext *video_en_ctx;
    AVFormatContext *ouFmtCtx;
    AVCodec *video_codec;
    
    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;

    float t, tincr, tincr2;
    
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
    void add_video_stream(AVFormatContext *oc,AVCodec **codec,enum AVCodecID codec_id);
    AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,uint64_t channel_layout,int sample_rate, int nb_samples);
    void open_video();
    AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    void fill_yuv_image(AVFrame *pict, int frame_index,int width, int height);
    AVFrame *get_video_frame();
    int write_video_frame(AVFormatContext *oc);
    void close_stream(AVFormatContext *oc);
};

#endif /* encodeMuxer_hpp */
