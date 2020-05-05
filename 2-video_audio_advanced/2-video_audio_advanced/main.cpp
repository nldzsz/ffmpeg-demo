//
//  main.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/3.
//  Copyright © 2020 apple. All rights reserved.
//

#include <iostream>

#include "transcode.hpp"

int main(int argc, const char * argv[]) {
    int test_use = 0;
    if (test_use == 0) {
        Transcode code;
        code.doTranscodeForMuxer();
    }
    
    return 0;
}
