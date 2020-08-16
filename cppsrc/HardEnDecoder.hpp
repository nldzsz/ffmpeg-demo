//
//  hardDecoder.hpp
//  video_encode_decode
//
//  Created by apple on 2020/4/22.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef hardDecoder_hpp
#define hardDecoder_hpp
#include <string>
#include <stdio.h>
#include "cppcommon/CLog.h"
#include <sys/time.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
#include <libavutil/error.h>
}
using namespace::std;

enum HardType{
    HardTypeVideoToolbox,
    HardTypeMediaCodec,
};
class HardEnDecoder
{
public:
    HardEnDecoder();
    ~HardEnDecoder();
    
    void doDecode(string srcPath,HardType type=HardTypeVideoToolbox);
    void doEncode(string srcPath,string dstPath,HardType type=HardTypeVideoToolbox);
};

#endif /* hardDecoder_hpp */
