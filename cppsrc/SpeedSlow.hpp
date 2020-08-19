//
//  SpeedSlow.hpp
//  demo-mac
//
//  Created by apple on 2020/8/10.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef SpeedSlow_hpp
#define SpeedSlow_hpp

#include <stdio.h>
#include <string>

using namespace std;
extern "C" {
#include "Common.h"
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}
class SpeedSlow
{
public:
    SpeedSlow();
    ~SpeedSlow();
    /** 丢
     */
    void doSpeedSlow(string spath,string dpath,int sp);
};
#endif /* SpeedSlow_hpp */

