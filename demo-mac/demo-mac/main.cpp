//
//  main.cpp
//  demo-mac
//
//  Created by apple on 2020/7/7.
//  Copyright © 2020 apple. All rights reserved.
//

#include <iostream>
#include "audio_resample.hpp"
#include "audio_encode.hpp"
#include "videoresample.hpp"
#include "demuxer.hpp"
#include "muxer.hpp"
#include "SoftEnDecoder.hpp"
#include "HardEnDecoder.hpp"
#include "transcode.hpp"
#include "encodeMuxer.hpp"
#include "Cut.hpp"
#include "Merge.hpp"
#include "VideoJpg.hpp"
#include "AudioVolume.hpp"
#include "VideoScale.hpp"
#include "audio_acrossfade.hpp"
#include "Subtitles.hpp"

int main(int argc, const char * argv[]) {
    string curFile(__FILE__);
    unsigned long pos = curFile.find("demo-mac");
    if (pos == string::npos) {
        return -1;
    }
    string resourceDir = curFile.substr(0,pos)+"filesources/";
    
    int test_use = 26;
    if (test_use == 0) {
        string pcmpath = resourceDir+"test_441_f32le_2.pcm";
        string dstpath = resourceDir+"test_441_f32le_2.aac";
        AudioEncode aEncode(pcmpath,dstpath);
        aEncode.doEncode(CodecFormatAAC,false);
    } else if (test_use == 1) {
        // 原pcm文件中存储的是float类型音频数据，小端序；packet方式
        string pcmpath = resourceDir+"test_441_f32le_2.pcm";
        string dstpath = resourceDir+"test_240_s32le_2.pcm";
        AudioResample aResample;
        aResample.doResample(pcmpath,dstpath);
    } else if (test_use == 2) {
        string srcpcm = resourceDir+"test_441_f32le_2.pcm";
        string dstpcm = resourceDir+"test_441_s32le_2.pcm";
        AudioResample aResample;
        aResample.doResampleAVFrame(srcpcm,dstpcm);
    } else if (test_use == 3) {
        string srcyuvPath = resourceDir + "test_640x360_yuv420p.yuv";
        string dstyuvPath = resourceDir + "test.yuv";
        VideoScale scale;
        scale.doScale(srcyuvPath,dstyuvPath);
    } else if (test_use == 4) {
        // mdata标签在moov之前
        string srcPath = resourceDir+"test-mp3-1.mp3";
        // mdata标签在moov之后
        //    string srcPath = "/Users/apple/Downloads/Screenrecorder-2020-03-31-16-36-12-749\(0\).mp4";
        Demuxer demuxer;
        demuxer.doDemuxer(srcPath);
    } else if (test_use == 5) {
        string aduio_srcPath = resourceDir + "test-mp3-1.mp3";
        //    string aduio_srcPath = resourceDir + "test_441_f32le_2.aac";
        string video_srcPath = resourceDir + "test_1280x720_2.mp4";
        string dstPath = resourceDir+"test.MOV";
        Muxer muxer;
        muxer.doMuxerTwoFile(aduio_srcPath,video_srcPath,dstPath);
    } else if (test_use == 6) {
//        string srcPath = resourceDir + "test_1280x720_1.mp4";
        string srcPath = resourceDir + "abc-test.h264";
        string dstPath = resourceDir + "1-test_1280_720_1.MP4";
        Muxer muxer;
        muxer.doReMuxer(srcPath,dstPath);
    } else if (test_use == 7) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        SoftEnDecoder decoder;
        decoder.doDecode(srcPath);
    } else if (test_use == 8) {
        string srcPath = resourceDir + "test_441_f32le_2.aac";
        SoftEnDecoder decoder;
        decoder.doDecode2(srcPath);
    } else if (test_use == 9) {
        string srcPath = resourceDir + "test_640x360_yuv420p.yuv";
        string dstPath = resourceDir+ "1-abc-test.h264";
        SoftEnDecoder decoder;
        decoder.doEncode(srcPath,dstPath);
    } else if (test_use == 10) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        HardEnDecoder decoder;
        decoder.doDecode(srcPath);
    } else if (test_use == 11) {
        string srcPath = resourceDir + "test_640x360_yuv420p.yuv";
        string dstPath = resourceDir + "3-test.h264";
        HardEnDecoder decoder;
        decoder.doEncode(srcPath,dstPath);
    } else if (test_use == 12) {
        string srcPath = resourceDir + "abc-test.h264";
        string dstPath = resourceDir + "2-test_1280_720_1.MP4";
        Muxer demuxer;
        demuxer.doReMuxerWithStream(srcPath,dstPath);
    } else if (test_use == 13) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        string dstPath = resourceDir + "2-test_1280x720_3.mov";   // 这里后缀可以改成flv,ts,avi mov则生成对应的文件
        Transcode codec;
        codec.doExtensionTranscode(srcPath,dstPath);
    } else if (test_use == 14) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        string dstPath = resourceDir + "1-test_1280x720_3.mov";
        Transcode codec;
        codec.doTranscode(srcPath,dstPath);
    } else if (test_use == 15) {
        string dstPath = resourceDir + "11-test.mp4";
        EncodeMuxer muxer;
        muxer.doEncodeMuxer(dstPath);
    } else if (test_use == 16) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        string dstPath = resourceDir + "1-cut-test_1280x720_3.mp4";
        string start = "00:00:15";
        int duration = 5;
        Cut cutObj;
        cutObj.doCut(srcPath,dstPath,start,duration);
    } else if (test_use == 17) {
//        string srcPath1 = resourceDir + "ll.mpg";
//        string srcPath2 = resourceDir + "lr.mpg";
//        string dstPath  = resourceDir + "1-merge_1.mpg";
        string srcPath1 = resourceDir + "test_1280x720_1.mp4";
        string srcPath2 = resourceDir + "test_1280x720_4.mp4";
        string dstPath  = resourceDir + "1-merge_1.mp4";
        Merge mObj;
        mObj.MergeTwo(srcPath1,srcPath2,dstPath);
    } else if (test_use == 18) {
        string srcPath1 = resourceDir + "test_1280x720_1.mp4";
        string srcPath2 = resourceDir + "test_1280x720_3.mp4";
        string dstpath  = resourceDir + "1-merge_1.mp4";
        Merge mObj;
        mObj.MergeFiles(srcPath1,srcPath2,dstpath);
    }
    else if (test_use == 19) {
        string srcpath  = resourceDir + "test_1280x720_4.mp4";
        //    string srcpath2 = srcDic + "test-mp3-1.mp3";
        string srcpath2 = resourceDir + "test_441_f32le_2.aac";
        string dstpath = resourceDir + "11_add_music.mp4";
        string start = "00:00:05";
        Merge mObj;
        mObj.addMusic(srcpath,srcpath2,dstpath,start);
    }
    else if (test_use == 20) {
        string srcPath = resourceDir + "test_1280x720_3.mp4";
        string dstPath = resourceDir + "1-doJpg_get%3d.jpg";
//        string dstPath = resourceDir + "1-doJpg_get.jpg";
        string start = "00:00:05";
        VideoJPG mObj;
        mObj.doJpgGet(srcPath,dstPath,start,true,5);
    }
    else if (test_use == 21) {
        string srcPath = resourceDir + "1-doJpg_get%3d.jpg";
        string dstPath = resourceDir + "1-dojpgToVideo.mp4";
        VideoJPG mObj;
        mObj.doJpgToVideo(srcPath,dstPath);
    }
    else if (test_use == 22) {
        string srcpath = resourceDir + "low_battery.mp3";
        string dstpath = resourceDir + "1-addaudiovolome-1.mp3";
        AudioVolume mObj;
        mObj.doChangeAudioVolume(srcpath,dstpath);
    }
    else if (test_use == 23) {
        string srcpath = resourceDir + "test-mp3-1.mp3";
        string dstpath = resourceDir + "1-audiovolume-2.mp3";
        AudioVolume mObj;
        mObj.doChangeAudioVolume2(srcpath,dstpath);
    }
    else if (test_use == 24) {
        string srcpath = resourceDir + "test_1280x720_3.mp4";
        string dstpath = resourceDir + "1-videoscale_1.mp4";
        FilterVideoScale mObj;
        mObj.doVideoScale(srcpath,dstpath);
    }
    else if (test_use == 25) {
        string apath1 = resourceDir + "test_mp3_1.mp3";
        string apath2 = resourceDir + "test_mp3_2.mp3";
        string dstpath = resourceDir + "1-output.mp3";
        AudioAcrossfade mObj;
        mObj.doAcrossfade(apath1, apath2, dstpath,10);
    }
    else if (test_use == 26) {
        string apath1 = resourceDir + "test_1280x720_3.mp4";
        string apath2 = resourceDir + "test_1280x720_3.srt";    // test_1280x720_3.ass
        string dstpath = resourceDir + "1-subtitles-out.mkv";
        Subtitles mObj;
        mObj.addSubtitleStream(apath1, apath2, dstpath);
    }
    
    return 0;
}
