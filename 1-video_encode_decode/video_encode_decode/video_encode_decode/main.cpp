//
//  main.cpp
//  video_encode_decode
//
//  Created by apple on 2019/12/5.
//  Copyright © 2019 apple. All rights reserved.
//

#include <iostream>
#include "videoresample.hpp"
#include "demuxer.hpp"
#include "muxer.hpp"

int main(int argc, const char * argv[]) {
    
    int test_use = 2;
    if (test_use == 0) {
        // 视频像素格式转换示例
        VideoScale scale;
        scale.doScale();
    } else if (test_use == 1) {
        Demuxer demuxer;
        demuxer.doDemuxer();
    } else if (test_use == 2) {
        Muxer muxer;
        muxer.doMuxerTwoFile();
    } else if (test_use == 3) {
        Muxer muxer;
        muxer.doReMuxer();
    }
    
    return 0;
}
