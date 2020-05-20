//
//  main.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/3.
//  Copyright © 2020 apple. All rights reserved.
//

#include <iostream>

#include "transcode.hpp"
#include "encodeMuxer.hpp"
#include "Cut.hpp"


int main(int argc, const char * argv[]) {
    
    int test_use = 3;
    if (test_use == 0) {
        Transcode codec;
        codec.doExtensionTranscode();
    } else if (test_use == 1) {
        Transcode codec;
        codec.doTranscode();
    } else if (test_use == 2) {
        EncodeMuxer muxer;
        muxer.doEncodeMuxer();
    } else if (test_use == 3) {
        Cut cutObj;
        cutObj.doCut();
    }
    
    return 0;
}
