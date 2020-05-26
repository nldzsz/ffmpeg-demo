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
#include "Merge.hpp"
#include "VideoJpg.hpp"


int main(int argc, const char * argv[]) {
    
    int test_use = 5;
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
    } else if (test_use == 4) {
        Merge mObj;
        mObj.MergeTwo();
    } else if (test_use == 5) {
        Merge mObj;
        mObj.MergeFiles();
    }
    else if (test_use == 6) {
        Merge mObj;
        mObj.addMusic();
    }
    
    return 0;
}
