//
//  audio_resample.hpp
//  audio_encode_decode
//
//  Created by apple on 2019/12/18.
//  Copyright © 2019 apple. All rights reserved.
//

#ifndef audio_resample_hpp
#define audio_resample_hpp

#include <stdio.h>
#include <string>
#include "cppcommon/CLog.h"

using namespace std;

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

/** 广义的音频的重采样包括：
 *  1、采样格式转化：比如采样格式从16位整形变为浮点型，对音质有一定影响
 *  2、采样率的转换：降采样和升采样，比如44100采样率降为2000采样率，对音质影响较大
 *  3、存放方式转化：音频数据从packet方式变为planner方式。有的硬件平台在播放声音时需要的音频数据是
 *  planner格式的，而有的可能又是packet格式的，或者其它需求原因经常需要进行这种存放方式的转化
 */
class AudioResample
{
public:
    AudioResample();
    ~AudioResample();
    
    void doResample();
private:
};

#endif /* audio_resample_hpp */
