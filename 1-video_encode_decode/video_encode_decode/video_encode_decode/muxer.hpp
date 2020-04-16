//
//  muxer.hpp
//  video_encode_decode
//
//  Created by apple on 2020/4/14.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef muxer_hpp
#define muxer_hpp

#include <stdio.h>
#include <string>

extern "C"{
#include <libavformat/avformat.h>
#include "CLog.h"
}
using namespace std;

class Muxer
{
public:
    /** ffmpeg编译完成后支持的封装器位于libavformat目录下的muxer_list.c文件中，具体的配置在.configure文件中，如下：
    *  print_enabled_components libavformat/muxer_list.c AVOutputFormat muxer_list $MUXER_LIST
    *  xxxx.mp4对应的封装器为ff_mov_muxer
    */
    Muxer();
    ~Muxer();
    void doMuxer();
};

#endif /* muxer_hpp */
