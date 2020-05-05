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

extern "C" {
#include "cppcommon/CLog.h"
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
using namespace std;

class Transcode
{
public:
    // 对封装容器进行转化，比如MP4,MOV,FLV,TS等容器之间的转换
    void doTranscodeForMuxer();
};

#endif /* transcode_h */
