//
//  CodecUtil.c
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/30.
//  Copyright © 2020 apple. All rights reserved.
//

#include "CodecUtil.h"

int select_sample_rate(AVCodec *codec,int rate)
{
    int best_rate = 0;
    int deft_rate = 44100;
    const int* p = codec->supported_samplerates;
    if (!p) {
        return deft_rate;
    }
    while (*p) {
        best_rate = *p;
        if (*p == rate) {
            break;
        }
        p++;
    }
    
    if (best_rate != rate && best_rate != 0 && best_rate != deft_rate) {
        return deft_rate;
    }
    
    return best_rate;
}

enum AVSampleFormat select_sample_format(AVCodec *codec,enum AVSampleFormat fmt)
{
    enum AVSampleFormat retfmt = AV_SAMPLE_FMT_NONE;
    enum AVSampleFormat deffmt = AV_SAMPLE_FMT_FLTP;
    const enum AVSampleFormat * fmts = codec->sample_fmts;
    if (!fmts) {
        return deffmt;
    }
    while (*fmts != AV_SAMPLE_FMT_NONE) {
        retfmt = *fmts;
        if (retfmt == fmt) {
            break;
        }
        fmts++;
    }
    
    if (retfmt != fmt && retfmt != AV_SAMPLE_FMT_NONE && retfmt != deffmt) {
        return deffmt;
    }
    
    return retfmt;
}

int64_t select_channel_layout(AVCodec *codec,int64_t ch_layout)
{
    int64_t retch = 0;
    int64_t defch = AV_CH_LAYOUT_STEREO;
    const uint64_t * chs = codec->channel_layouts;
    if (!chs) {
        return defch;
    }
    while (*chs) {
        retch = *chs;
        if (retch == ch_layout) {
            break;
        }
        chs++;
    }
    
    if (retch != ch_layout && retch != AV_SAMPLE_FMT_NONE && retch != defch) {
        return defch;
    }
    
    return retch;
}

enum AVPixelFormat select_pixel_format(AVCodec *codec,enum AVPixelFormat fmt)
{
    enum AVPixelFormat retpixfmt = AV_PIX_FMT_NONE;
    enum AVPixelFormat defaltfmt = AV_PIX_FMT_YUV420P;
    const enum AVPixelFormat *fmts = codec->pix_fmts;
    if (!fmts) {
        return defaltfmt;
    }
    while (*fmts != AV_PIX_FMT_NONE) {
        retpixfmt = *fmts;
        if (retpixfmt == fmt) {
            break;
        }
        fmts++;
    }
    
    if (retpixfmt != fmt && retpixfmt != AV_PIX_FMT_NONE && retpixfmt != defaltfmt) {
        return defaltfmt;
    }
    
    return retpixfmt;
}
