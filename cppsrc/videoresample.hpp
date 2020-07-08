//
//  videoresample.hpp
//  video_encode_decode
//
//  Created by apple on 2020/3/24.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef videoresample_hpp
#define videoresample_hpp

#include <stdio.h>
#include <string>
#include "cppcommon/CLog.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}
using namespace std;

typedef enum
{
    FormatTypeYUV420P,
    FormattypeRGB24
}FormatType;

class VideoScale {
public:
    VideoScale();
    ~VideoScale();
    
    /** sws_scale()函数主要用来进行视频的缩放和视频数据的格式转换。对于缩放，有不同的算法，每个算法的性能不一样
     */
    void doScale(string srcpath,string dstpath);
};


#endif /* videoresample_hpp */
