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
#include "SoftEnDecoder.hpp"
#include "HardEnDecoder.hpp"

int main(int argc, const char * argv[]) {
    
    int test_use = 8;
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
    } else if (test_use == 4) {
       SoftEnDecoder decoder;
       decoder.doDecode();
    } else if (test_use == 5) {
       SoftEnDecoder decoder;
       decoder.doDecode2();
    } else if (test_use == 6) {
        SoftEnDecoder decoder;
        decoder.doEncode();
    } else if (test_use == 7) {
        HardEnDecoder decoder;
        decoder.doDecode();
    } else if (test_use == 8) {
        HardEnDecoder decoder;
        decoder.doEncode();
    }
    
    return 0;
}
