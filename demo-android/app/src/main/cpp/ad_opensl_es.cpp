//
// Created by 飞拍科技 on 2019/5/22.
//

// for jni
#include <jni.h>
// for c++ 要引入，否则NULL关键字找不到
#include <stdio.h>
#include <memory.h>

// for log
extern "C" {
#include "Common.h"
}

#include "SLAudioPlayer.h"
#include "opensles/SLAudioPlayer.h"

int willexit = 0;
FILE *pcmFile;
SLAudioPlayer *player;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_demo_Audio_ADOpenSLES_playAudio(JNIEnv *env, jobject instance, jstring path_,
                                          jint sample_rate, jint ch_layout, jint format) {
    const char *path = env->GetStringUTFChars(path_, 0);
    willexit = 0;
    pcmFile = fopen(path,"r");

    if (pcmFile == NULL) {
        LOGD("file is null");
        env->ReleaseStringUTFChars(path_, path);
        return;
    }

    // 一次读取20ms的采样
    int perid = 20; // 20ms
    int chs = getChannel_layout_Channels((Channel_Layout)ch_layout);
    int bufSamples = chs * sample_rate * perid / 1000;    // 表示20ms 有多少个采样

    player = new SLAudioPlayer(bufSamples,sample_rate,(Sample_format)format,(Channel_Layout)ch_layout);
    player->openAudioPlayer();

    LOGD("开始读取音频数据 %d",format);


    if (format == Sample_format_SignedInteger_8) {
        char buffer[bufSamples];
        while (!willexit && !feof(pcmFile)) {
            if (fread((char *)buffer, bufSamples, 1, pcmFile) != 1) {
                LOGD("failed to read data \n ");
                break;
            }
            player->putAudioData(buffer,bufSamples);
        }
    } else if(format == Sample_format_SignedInteger_16) {
        int16_t buffer[bufSamples];
        while (!willexit && !feof(pcmFile)) {
            if (fread((char *)buffer, bufSamples* sizeof(int16_t), 1, pcmFile) != 1) {
                LOGD("failed to read data \n ");
                break;
            }
            player->putAudioData((char *)buffer,bufSamples);
            LOGD("读取到了 %d 个sample",bufSamples);
        }
    } else if (format == Sample_format_SignedInteger_32) {
        int32_t buffer[bufSamples];
        while (!willexit && !feof(pcmFile)) {
            if (fread((char *)buffer, bufSamples* sizeof(int32_t), 1, pcmFile) != 1) {
                LOGD("failed to read data \n ");
                break;
            }
            player->putAudioData((char *)buffer,bufSamples);
        }
    }
    LOGD("播放结束");

    player->closeAudioPlayer();
    player = NULL;
    fclose(pcmFile);
    pcmFile = NULL;

    env->ReleaseStringUTFChars(path_, path);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_demo_Audio_ADOpenSLES_stopAudio(JNIEnv *, jobject) {
    willexit = 1;
}
