//
//  CodecUtil.h
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/30.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef CodecUtil_h
#define CodecUtil_h

#include <stdio.h>
#include <libavcodec/avcodec.h>

/**
 *  1、如果音频编码器支持rate指定的采样率，则返回值为rate
 *  2、如果音频编码器的suported_samplerates为NULL或者不支持rate指定的采样率，则返回44100；
 */
extern int select_sample_rate(AVCodec *codec,int rate);

/**
 *  1、如果音频编码器支持fmt指定的采样格式，则返回值为fmt
 *  2、如果音频编码器的sample_fmts为NULL或者不支持fmt指定的采样格式，则返回AV_SAMPLE_FMT_FLTP；
 */
enum AVSampleFormat select_sample_format(AVCodec *codec,enum AVSampleFormat fmt);

/**
 *  1、如果音频编码器支持ch_layout指定的声道类型，则返回值为ch_layout
 *  2、如果音频编码器的channel_layouts为NULL或者不支持ch_layout指定的声道类型，则返回AV_CH_LAYOUT_STEREO；
 */
int64_t select_channel_layout(AVCodec *codec,int64_t ch_layout);

/**
 *  1、如果视频编码器支持fmt指定的像素格式，则返回值为fmt
 *  2、如果视频编码器的pix_fmts为NULL或者不支持fmt指定的像素格式，则返回AV_PIX_FMT_YUV420P；
 */
enum AVPixelFormat select_pixel_format(AVCodec *codec,enum AVPixelFormat fmt);

#endif /* CodecUtil_h */
