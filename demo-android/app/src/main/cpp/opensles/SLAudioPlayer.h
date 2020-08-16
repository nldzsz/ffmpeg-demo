//
// Created by 飞拍科技 on 2019/5/23.
//

#ifndef MEDIA_ANDROID_SLAUDIOPLAYER_H
#define MEDIA_ANDROID_SLAUDIOPLAYER_H

#include <pthread.h>
#include <stdlib.h>

extern "C" {
#include "CLog.h"
};
#include "OpenSLESContext.h"
#include "openslutil.h"
#define PERIOD 20   // 20ms 每次向缓冲区发送20ms的采样数据

/** 备注：安卓设备貌似对8位32位音频无法播放
 * */
class SLAudioPlayer {
private:
    OpenSLESContext *slContext;

    // 采样率
    Sample_rate fSample_rate;
    // 采样格式
    Sample_format fSample_format;
    // 声道类型
    Channel_Layout fChannel_layout;

    int currentOutputIndex;     // 目前使用缓冲区的填充位置索引
    int currentOutputBuffer;    // 目前使用的是哪个缓冲区的索引
    char *outputBuffer[2];   // 总共两个缓冲区，用来填充应用端的数据
    int outBufSamples;          // 每一个缓冲区最多填充的采样数

    // 混音器
    SLObjectItf outputMixObject;


    SLObjectItf playerObject;   // 音频播放组件
    SLPlayItf   playerInf;      // 音频播放组件的音频播放接口
    SLVolumeItf volInf;         // 音频播放组件的音量控制接口
    SLAndroidSimpleBufferQueueItf pcmBufferQueueItf; // 音频播放组件的缓冲区接口

    // 保证线程安全，条件变量进行数据的通知
    pthread_mutex_t mutex_out;
    pthread_cond_t  cont_out;
    int             cont_val;

    void initBuffers();
    void destroyBuffers();
    void initThreadLock();
    void destroyThreadLock();
    void waitThreadLock();
    void notifyThreadLock();

    // 回调函数
    static void pcmBufferCallBack(SLAndroidSimpleBufferQueueItf bf, void * context);

public:
    SLAudioPlayer(int bufSamples,Sample_rate rate,Sample_format format,Channel_Layout ch);
    virtual ~SLAudioPlayer();

    // 初始化并开始播放音频
    void openAudioPlayer();

    // 关闭音频播放器并释放资源
    void closeAudioPlayer();

    // 往OPenSL ES输送音频数据，音频数据格式根据采样格式而定
    void putAudioData(char * buffer,int size);
};


#endif //MEDIA_ANDROID_SLAUDIOPLAYER_H
