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
    string curFile(__FILE__);
    unsigned long pos = curFile.find("ffmpeg-demo");
    if (pos == string::npos) {  // 未找到
        return 0;
    }
    string resouceDic = curFile.substr(0,pos)+"ffmpeg-demo/filesources/";
    
    string pcmpath = resouceDic+"test_441_f32le_2.pcm";
    

    // 编码
//    AudioEncode aEncode;
//    string dstpath2 = "test_441_f32le_2.aac";
//    string dstpath3 = "test_441_f32le_3.aac";
//    aEncode.doEncode_flt32_le2_to_aac1(pcmpath, dstpath2);
//    aEncode.doEncode_flt32_le2_to_aac2(pcmpath, dstpath3);
    
    
    // 音频重采样
    AudioResample aResample;
    aResample.doResample();
    
    return 0;
}
