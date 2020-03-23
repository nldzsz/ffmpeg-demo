//
//  main.cpp
//  audio_encode_decode
//
//  Created by apple on 2019/11/29.
//  Copyright © 2019 apple. All rights reserved.
//

#include <iostream>
#include "audio_encode.hpp"
#include "audio_resample.hpp"

using namespace std;

int main(int argc, const char * argv[]) {
    
    // 编码
    AudioEncode aEncode;
    aEncode.doEncode(CodecFormatAC3,false);
    
    
    // 音频重采样
    AudioResample aResample;
//    aResample.doResample();
//    aResample.doResampleAVFrame();
    
    return 0;
}
