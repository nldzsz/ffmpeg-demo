//
//  SoftDecoder.hpp
//  video_encode_decode
//
//  Created by apple on 2020/4/20.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef SoftDecoder_hpp
#define SoftDecoder_hpp

#include <stdio.h>
#include <string>
#include "cppcommon/CLog.h"
using namespace std;

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
class SoftEnDecoder
{
public:
    SoftEnDecoder();
    ~SoftEnDecoder();
    // 解码本地MP4等封装后的格式文件
    void doDecode();
    // 解码aac/h264流的文件
    void doDecode2();
    // 编码yuv为h264
    void doEncode();
};
#endif /* SoftDecoder_hpp */
