//
//  audio_resample.cpp
//  audio_encode_decode
//
//  Created by apple on 2019/12/18.
//  Copyright © 2019 apple. All rights reserved.
//

#include "audio_resample.hpp"

AudioResample::AudioResample()
{
    
}

AudioResample::~AudioResample()
{
    
}

void AudioResample::doResample()
{
    string curFile(__FILE__);
    unsigned long pos = curFile.find("ffmpeg-demo");
    if (pos == string::npos) {  // 未找到
        return;
    }
    string resouceDic = curFile.substr(0,pos)+"ffmpeg-demo/filesources/";
    
    // 原pcm文件中存储的是float类型音频数据，小端序；packet方式
    string pcmpath = resouceDic+"test_441_f32le_2.pcm";
    string dstpath1 = "test_441_f32le_2.pcm";
    
    FILE *srcFile = fopen(pcmpath.c_str(), "rb+");
    if (srcFile == NULL) {
        LOGD("fopen fail");
        return;
    }
    
    // 音频数据
    uint8_t **src_data,**dst_data;
    // 声道数
    int src_nb_channels=0,dst_nb_channels=0;
    int64_t src_ch_layout = AV_CH_LAYOUT_STEREO,dst_ch_layout = AV_CH_LAYOUT_STEREO;
    // 采样率
    int64_t src_rate = 44100,dst_rate = 48000;
    // 采样格式和音频数据存储方式
    const enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT,dst_sample_fmt = AV_SAMPLE_FMT_DBLP;
    int src_nb_samples=1024,dst_nb_samples,max_dst_nb_samples;
    int src_linesize,dst_linesize;
    
    SwrContext *swrCtx;
    // 1、创建上下文
    swrCtx = swr_alloc();
    if (swrCtx == NULL) {
        LOGD("swr_allock fail");
        return;
    }
    
    // 2、设置重采样的相关参数
    av_opt_set_int(swrCtx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swrCtx, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", src_sample_fmt, 0);
    
    av_opt_set_int(swrCtx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swrCtx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", dst_sample_fmt, 0);
    
    // 3、初始化上下文
    int ret = 0;
    ret = swr_init(swrCtx);
    if (ret < 0) {
        LOGD("swr_init fail");
        return;
    }
    
    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    // 根据src_sample_fmt、src_nb_samples、src_nb_channels为src_data分配内存空间，和设置对应的的linesize的值；返回分配的总内存的大小
    int src_buf_size = av_samples_alloc_array_and_samples((uint8_t***)&src_data, &src_linesize, src_nb_channels, src_nb_samples, src_sample_fmt, 0);
    // 根据src_nb_samples*dst_rate/src_rate公式初步估算重采样后音频的nb_samples大小
    max_dst_nb_samples = dst_nb_samples = (int)av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
    int dst_buf_size = av_samples_alloc_array_and_samples((uint8_t***)&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 0);
    
    size_t read_size = 0;
    while ((read_size = fread(src_data[0], 1, src_buf_size, srcFile)) > 0) {
        
        /** 再次估算重采样后的的nb_samples的大小，这里swr_get_delay()用于获取重采样的缓冲延迟
         *  dst_nb_samples的值会经过多次调整后区域稳定
         */
        dst_nb_samples = (int)av_rescale_rnd(swr_get_delay(swrCtx, src_rate)+src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            LOGD("要重新分配内存了");
            // 先释放以前的内存
            av_freep(&dst_data[0]);
            // 再重新分配内存
            dst_buf_size = av_samples_alloc_array_and_samples((uint8_t***)&dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 0);
            max_dst_nb_samples = dst_nb_samples;
        }

        // 开始重采样
        int result = swr_convert(swrCtx, dst_data, dst_nb_samples, (const uint8_t **)src_data, src_nb_samples);
        if (result < 0) {
            LOGD("swr_convert fail %d",result);
        }
        LOGD("dst_nb_samples %d src_nb_samples %d",dst_nb_samples,src_nb_samples);
        
    }
    
    // 释放资源
    av_freep(&src_data[0]);
    av_freep(&dst_data[0]);
    swr_free(&swrCtx);
}
