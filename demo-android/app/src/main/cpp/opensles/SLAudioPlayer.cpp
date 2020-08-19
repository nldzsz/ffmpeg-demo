//
// Created by 飞拍科技 on 2019/5/23.
//

#include "SLAudioPlayer.h"
#include <memory.h>


SLAudioPlayer::SLAudioPlayer(int bufSamples,Sample_rate rate, Sample_format format, Channel_Layout ch)
        :outBufSamples(bufSamples),fSample_rate(rate),fSample_format(format),fChannel_layout(ch)
{
    LOGD("SLAudioPlayer() rate=%d format(%d) ch(%d)",rate,format,ch);
    slContext = new OpenSLESContext();
    initBuffers();
    initThreadLock();
}
SLAudioPlayer::~SLAudioPlayer() {

}

void SLAudioPlayer::pcmBufferCallBack(SLAndroidSimpleBufferQueueItf, void * context)
{
    LOGD("目前线程 %ld",pthread_self());
    SLAudioPlayer *ap = (SLAudioPlayer*)context;
    ap->notifyThreadLock();
}

void SLAudioPlayer::openAudioPlayer() {
    SLresult  result;

    // 第一步创建引擎接口对象
    SLEngineItf engineItf = slContext->getEngineObject();

    /** 第二步 创建混音器;她可能不仅仅代表的是声音效果器，它默认是进行音频的播放输出，音频播放必不可少的组件;
     * CreateOutputMix()参数说明如下:
     * 参数1:引擎接口对象
     * 参数2:混音器对象地址
     * 参数3:组件的可配置属性ID个数;如果为0后面两个参数忽略；不同的组件，所拥有的属性种类和个数也不一样，如果某个组件不支持某个属性，GetInterface将
     * 返回SL_RESULT_FEATURE_UNSUPPORTED
     * 参数4:需要配置的属性ID,数组
     * 参数5:配置的这些属性ID对于组件来说是否必须,数组，与参数4一一对应
     * */
    // 这里只是进行简单的音频播放输出，所以不创建任何混音效果的属性ID，第三个参数传0即可
    result = (*engineItf)->CreateOutputMix(engineItf,&outputMixObject,0,NULL,NULL);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("CreateOutputMix fail %d",result);
    }
    // 实例化混响器，创建功能组件类型后还需要实例化;OpenSL ES
    result = (*outputMixObject)->Realize(outputMixObject,SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("Realize outputMixObject fail %d",result);
    }

    /** SLDataLocator_AndroidSimpleBufferQueue 表示数据缓冲区的结构，用来表示一块缓冲区
     * 1、第一个参数必须是SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE
     * 2、第二个参数表示队列中缓冲区的个数，这里测试1 2 3 4都可以正常。
     * */
    SLDataLocator_AndroidSimpleBufferQueue android_queue={SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,2};
    SLuint32 chs = getChannel_layout_Channels(fChannel_layout);
    SLuint32 ch = getChannel_layout_Type(fChannel_layout);
    SLuint32 sr = getSampleRate(fSample_rate);
    SLuint32 fo = getPCMSample_format(fSample_format);
    LOGD("声道数 %d 声道类型 %d 采样率 %d 采样格式 %d",chs,ch,sr,fo);
    SLDataFormat_PCM pcm={
            SL_DATAFORMAT_PCM,//播放pcm格式的数据
            chs,//声道个数
            sr,//采样率
            fo,//位数
            fo,//和位数一致就行
            ch,//立体声（前左前右）
            SL_BYTEORDER_LITTLEENDIAN//数据存储是小端序
    };
    /** SLDataSource 表示输入缓冲区，和输出缓冲区一样，它由数据类型和数据格式组成
     * 1、参数1;指向指定的数据缓冲区，SLDataLocator_xxx结构体定义，可取值如下:
     *      SLDataLocator_Address
     *      SLDataLocator_BufferQueue   (一块数据缓冲区，对于安卓来说是SLDataLocator_AndroidSimpleBufferQueue)
     *      SLDataLocator_IODevice
     *      SLDataLocator_MIDIBufferQueue
     *      SLDataLocator_URI
     * 2、参数2;表示缓冲区中数据的格式，可取值如下:
     *      SLDataFormat_PCM
     *      SLDataFormat_MIME
     *   其中如果第一个参数是SLDataLocator_IODevice，此参数忽略,传NULL即可
     * */
    SLDataSource slDataSource = {&android_queue, &pcm};

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX,outputMixObject};
    /** SLDataSink 表示了输出缓冲区，包括数据类型和数据格式
     * 1、参数1;指向指定的数据缓冲区类型，一般由SLDataLocator_xxx结构体定义，可取值如下:
     *      SLDataLocator_Address
     *      SLDataLocator_IODevice
     *      SLDataLocator_OutputMix         (混音器，代表着音频输出)
     *      SLDataLocator_URI
     *      SLDataLocator_BufferQueue       (一块数据缓冲区，对于安卓来说是SLDataLocator_AndroidSimpleBufferQueue)
     *      SLDataLocator_MIDIBufferQueue
     * 2、参数2;表示缓冲区中数据的格式，可取值如下:
     *      SLDataFormat_PCM
     *      SLDataFormat_MIME
     *    其中如果第一个参数是SLDataLocator_IODevice或SLDataLocator_OutputMix，此参数忽略,传NULL即可
     * 住：我们要做的只是给定缓冲区类型和缓冲区中数据格式即可，系统自动为我们分配内存
     * */
    SLDataSink audioSink = {&outputMix, NULL};


    /** 第三步，创建AudioPlayer组件;该组件必须要有输入数据缓冲区(提供要播放的音频数据)，输出数据缓冲区(硬件最终从这里获取数据)、
     * 一个音频数据中间缓冲区(由于应用无法直接向输入数据缓冲区写入数，是通过这个缓冲区间接写入的，所以必须要有这样一个缓冲区)和可选属性(比如控制音量等等)
     *  参数1:openSL es引擎接口
     *  参数2:AudioPlayer组件对象
     *  参数3:输入缓冲区地址
     *  参数4:输出缓冲区地址
     *  参数5:属性ID个数
     *  参数6:属性ID数组
     *  参数7:属性ID是否必须数组
     *  备注：可以看到 输入和输出缓冲区是通过参数3和4直接配置，而音频数据中间缓冲区是通过属性ID方式配置
     *
     *  audio player的数据驱动流程为：首先播放系统从audioSnk要数据进行播放，而audioSnk又从audioSrc要音频数据，audioSrc中数据是通过ids1中配置的回调函数不停的往
     *  其中写入数据，这样整个播放流程就理顺了。
     *  1、CreateAudioPlayer()第五个参数不能为0，否则audioSrc将没有数据送给audioSnk
     *  2、要写入数据的回调函数在单独的线程中，大概每12ms-20ms定期调用。
     * */
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*engineItf)->CreateAudioPlayer(engineItf, &playerObject, &slDataSource, &audioSink, 1, ids, req);
    //初始化播放器
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE);

//    注册回调缓冲区 获取缓冲队列接口
    result = (*playerObject)->GetInterface(playerObject, SL_IID_BUFFERQUEUE, &pcmBufferQueueItf);
    //注册缓冲接口回调，开始播放后，此函数将
    result = (*pcmBufferQueueItf)->RegisterCallback(pcmBufferQueueItf, SLAudioPlayer::pcmBufferCallBack, this);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("RegisterCallback fail %d",result);
    }

    // 获取音量接口 用于设置音量
//    (*playerObject)->GetInterface(playerObject, SL_IID_VOLUME, &volInf);
//    (*volInf)->SetVolumeLevel(volInf,100*50);

    //    得到接口后调用  获取Player接口
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerInf);
//    开始播放
    result = (*playerInf)->SetPlayState(playerInf, SL_PLAYSTATE_PLAYING);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("SetPlayState fail %d",result);
    }
    LOGD("结束");
    notifyThreadLock();
}

/** 释放 OpenSL ES资源，
 *  由于OpenSL ES的内存是由系统自动分配的，所以释放内存时只需要调用Destroy释放对应的SLObjectItf对象即可，然后它的属性ID接口对象直接置为NULL
 * */
void SLAudioPlayer::closeAudioPlayer() {
    LOGD("closeAudioPlayer()");
    // 释放音频播放器组件对象
    if (playerObject != NULL) {
        (*playerObject)->Destroy(playerObject);
        playerObject = NULL;
        playerInf = NULL;
        volInf = NULL;
        pcmBufferQueueItf = NULL;
    }

    // 释放混音器组件对象
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }

    // 释放OpenSL ES引擎对象
    if (slContext != NULL) {
        slContext->releaseResources();
        slContext = NULL;
    }

    destroyBuffers();
    destroyThreadLock();
}

/**  outBufSamples:是一个固定的数值；表示每次发往音频数据缓冲区的采样个数
  *  outputBuffer:是一个包含两个缓冲区的内存块，这是一个巧妙的设计。即保证了不频繁创建内存，又保证了内存中数据的安全。(具体过程为:当一个缓冲区数据通过Enqueue
  *  函数输送给OpenSL ES时，那么应用端再没有收到OpenSL ES缓冲区回调接口需要数据的回调时，此内存块就不能再使用，所以就用另外一块缓冲区，如果另外一块缓冲区也被
  *  应用填满了数据还没有收到OpenSL ES缓冲区回调接口需要数据的回调怎么办？说明OpenSL ES比较忙，暂时不需要数据，应用端一直阻塞知直到收到通知)
  *  currentOutputIndex:表示目前应用端填充数据的缓冲区下个填充数据位置
  *  currentOutputBuffer:表示目前应用端使用的是哪一个缓冲区
 **/
void SLAudioPlayer::putAudioData(char * buff,int size)
{
    // 从上一次位置开始填充数据
    int indexOfOutput = currentOutputIndex;

    if (fSample_format == Sample_format_SignedInteger_8) {
        char *newBuffer = (char *)buff;
        char *useBuffer = outputBuffer[currentOutputBuffer];

        for (int i = 0; i < size; ++i) {
            useBuffer[indexOfOutput++] = newBuffer[i];
            if (indexOfOutput >= outBufSamples) {   // 说明填满了一个缓冲区，则发送给OpenSL ES

                // 发送之前需要得到可以发送的条件
                waitThreadLock();

                // 发送数据
                (*pcmBufferQueueItf)->Enqueue(pcmBufferQueueItf,useBuffer,outBufSamples* sizeof(char));

                // 换成另外一个缓冲区
                currentOutputBuffer = currentOutputBuffer?0:1;
                indexOfOutput = 0;
                useBuffer = outputBuffer[currentOutputBuffer];
            }
        }

        // 更新上一次位置
        currentOutputIndex = indexOfOutput;

    } else if (fSample_format == Sample_format_SignedInteger_16) {
        short *newBuffer = (short *)buff;
        short *useBuffer = (short *)outputBuffer[currentOutputBuffer];

        for (int i = 0; i < size; ++i) {
            useBuffer[indexOfOutput++] = newBuffer[i];
            if (indexOfOutput >= outBufSamples) {   // 说明填满了一个缓冲区，则发送给OpenSL ES
                LOGD("被阻塞");
                // 发送之前需要得到可以发送的条件
                waitThreadLock();
                LOGD("阻塞完成");
                // 发送数据
                (*pcmBufferQueueItf)->Enqueue(pcmBufferQueueItf,useBuffer,outBufSamples* sizeof(short));

                // 换成另外一个缓冲区
                currentOutputBuffer = currentOutputBuffer?0:1;
                indexOfOutput = 0;
                useBuffer = (short *)outputBuffer[currentOutputBuffer];
            }
        }

        // 更新上一次位置
        currentOutputIndex = indexOfOutput;

    } else if (fSample_format == Sample_format_SignedInteger_32) {
        int *newBuffer = (int *)buff;
        int *useBuffer = (int *)outputBuffer[currentOutputBuffer];

        for (int i = 0; i < size; ++i) {
            useBuffer[indexOfOutput++] = newBuffer[i];
            if (indexOfOutput >= outBufSamples) {   // 说明填满了一个缓冲区，则发送给OpenSL ES

                // 发送之前需要得到可以发送的条件
                waitThreadLock();

                // 发送数据
                (*pcmBufferQueueItf)->Enqueue(pcmBufferQueueItf,useBuffer,outBufSamples* sizeof(int));

                // 换成另外一个缓冲区
                currentOutputBuffer = currentOutputBuffer?0:1;
                indexOfOutput = 0;
                useBuffer = (int*)outputBuffer[currentOutputBuffer];
            }
        }

        // 更新上一次位置
        currentOutputIndex = indexOfOutput;
    }
}

void SLAudioPlayer::initBuffers()
{
    if(outBufSamples != 0) {
        if (fSample_format == Sample_format_SignedInteger_8) {
            outputBuffer[0] = (char *) calloc((size_t)outBufSamples, sizeof(char));
            outputBuffer[1] = (char *) calloc((size_t)outBufSamples, sizeof(char));
        } else if (fSample_format == Sample_format_SignedInteger_16) {
            outputBuffer[0] = (char *) calloc((size_t)outBufSamples, sizeof(short));
            outputBuffer[1] = (char *) calloc((size_t)outBufSamples, sizeof(short));
        } else if (fSample_format == Sample_format_SignedInteger_32) {
            outputBuffer[0] = (char *) calloc((size_t)outBufSamples, sizeof(int));
            outputBuffer[1] = (char *) calloc((size_t)outBufSamples, sizeof(int));
        }
    }

    currentOutputBuffer  = 0;
    currentOutputIndex = 0;
}

void SLAudioPlayer::destroyBuffers()
{
    if (outputBuffer[0] != NULL) {
        free(outputBuffer[0]);
        outputBuffer[0] = NULL;
    }

    if (outputBuffer[1] != NULL) {
        free(outputBuffer[1]);
        outputBuffer[1] = NULL;
    }
}

void SLAudioPlayer::initThreadLock()
{
    pthread_mutex_init(&mutex_out, NULL);
    pthread_cond_init(&cont_out,NULL);
}
void SLAudioPlayer::destroyThreadLock()
{
    pthread_mutex_destroy(&mutex_out);
    pthread_cond_destroy(&cont_out);
}
void SLAudioPlayer::waitThreadLock()
{
    pthread_mutex_lock(&mutex_out);
    if (cont_val == 0) {
        pthread_cond_wait(&cont_out,&mutex_out);
    }
    cont_val = 0;
    pthread_mutex_unlock(&mutex_out);
}
void SLAudioPlayer::notifyThreadLock()
{
    pthread_mutex_lock(&mutex_out);
    cont_val = 1;
    pthread_cond_signal(&cont_out);
    pthread_mutex_unlock(&mutex_out);
}