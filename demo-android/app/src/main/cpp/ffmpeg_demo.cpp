//
// Created by 飞拍科技 on 2019/5/22.
//

// for jni
#include <jni.h>
// for c++ 要引入，否则NULL关键字找不到
#include <stdio.h>
#include <memory.h>
#include <string>
#include <stdarg.h>

// for log
extern "C" {
#include "Common.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/jni.h"
}
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

extern "C" jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jint result = -1;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        LOGD("JNI version not supported.");
        return JNI_ERR;
    }

    av_jni_set_java_vm(vm,reserved);

    return JNI_VERSION_1_6;
}

static void custom_log_callback(void *ptr,int level, const char* format,va_list val)
{
    if (level > av_log_get_level()) {
        return;
    }
    char out[200] = {0};
    int prefixe = 0;
    av_log_format_line2(ptr,level,format,val,out,200,&prefixe);
    LOGD("%s",out);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_testFFmpeg(JNIEnv *env, jobject instance)
{
    AVCodec *codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        LOGD("not found libx264");
    } else {
        LOGD("yes found libx264");
    }

    AVCodec *codec1 = avcodec_find_encoder_by_name("libmp3lame");
    if (!codec1) {
        LOGD("not found libmp3lame");
    } else {
        LOGD("yes found libmp3lame");
    }

    AVCodec *codec2 = avcodec_find_encoder_by_name("libfdk_aac");
    if (!codec2) {
        LOGD("not found libfdk_aac");
    } else {
        LOGD("yes found libfdk_aac");
    }

    // 打印ffmpeg日志
    av_log_set_level(AV_LOG_QUIET);
    av_log_set_callback(custom_log_callback);
}
std::string jstring2string(JNIEnv *env, jstring jStr){
    const char *cstr = env->GetStringUTFChars(jStr, NULL);
    std::string str = std::string(cstr);
    env->ReleaseStringUTFChars(jStr, cstr);
    return str;
}

extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doEncodecPcm2aac(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    AudioEncode aEncode(pcmpath,dpath);
    aEncode.doEncode(CodecFormatAAC,false);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doResample(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    AudioResample aResample;
    aResample.doResample(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doResampleAVFrame(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    AudioResample aResample;
    aResample.doResampleAVFrame(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doScale(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);
    VideoScale scale;
    scale.doScale(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doDemuxer(JNIEnv *env, jobject instance,jstring srcpath)
{
    string pcmpath = jstring2string(env,srcpath);

    Demuxer demuxer;
    demuxer.doDemuxer(pcmpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doMuxerTwoFile(JNIEnv *env, jobject instance,jstring apath,jstring vpath,jstring dstpath)
{
    string audio_path = jstring2string(env,apath);
    string video_vpath = jstring2string(env,vpath);
    string dpath = jstring2string(env,dstpath);

    Muxer muxer;
    muxer.doMuxerTwoFile(audio_path,video_vpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doReMuxer(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    Muxer muxer;
    muxer.doReMuxer(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doSoftDecode(JNIEnv *env, jobject instance,jstring srcpath)
{
    string pcmpath = jstring2string(env,srcpath);
    SoftEnDecoder decoder;
    decoder.doDecode(pcmpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doSoftDecode2(JNIEnv *env, jobject instance,jstring srcpath)
{
    string pcmpath = jstring2string(env,srcpath);

    SoftEnDecoder decoder;
    decoder.doDecode2(pcmpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doSoftEncode(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    SoftEnDecoder decoder;
    decoder.doEncode(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doHardDecode(JNIEnv *env, jobject instance,jstring srcpath)
{
    string pcmpath = jstring2string(env,srcpath);

    HardEnDecoder decoder;
    decoder.doDecode(pcmpath,HardTypeMediaCodec);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doHardEncode(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    HardEnDecoder decoder;
    decoder.doEncode(pcmpath,dpath,HardTypeMediaCodec);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doReMuxerWithStream(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    Muxer demuxer;
    demuxer.doReMuxerWithStream(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doExtensionTranscode(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    Transcode codec;
    codec.doExtensionTranscode(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doTranscode(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    Transcode codec;
    codec.doTranscode(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doEncodeMuxer(JNIEnv *env, jobject instance,jstring dstpath)
{
    string dpath = jstring2string(env,dstpath);

    EncodeMuxer muxer;
    muxer.doEncodeMuxer(dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doCut(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath,jstring start,jint du)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);
    string st = jstring2string(env,start);

    Cut cutObj;
    cutObj.doCut(pcmpath,dpath,st,du);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_MergeTwo(JNIEnv *env, jobject instance,jstring srcpath1,jstring srcpath2,jstring dstpath)
{
    string pcmpath1 = jstring2string(env,srcpath1);
    string pcmpath2 = jstring2string(env,srcpath2);
    string dpath = jstring2string(env,dstpath);
    Merge mObj;
    mObj.MergeTwo(pcmpath1,pcmpath2,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_MergeFiles(JNIEnv *env, jobject instance,jstring srcpath1,jstring srcpath2,jstring dstpath)
{
    string pcmpath1 = jstring2string(env,srcpath1);
    string pcmpath2 = jstring2string(env,srcpath2);
    string dpath = jstring2string(env,dstpath);

    Merge mObj;
    mObj.MergeFiles(pcmpath1,pcmpath2,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_addMusic(JNIEnv *env, jobject instance,jstring vpath,jstring apath,jstring dstpath,jstring st)
{
    string pcmpath1 = jstring2string(env,vpath);
    string pcmpath2 = jstring2string(env,apath);
    string dpath = jstring2string(env,dstpath);
    string start = jstring2string(env,st);

    Merge mObj;
    mObj.addMusic(pcmpath1,pcmpath2,dpath,start);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doJpgGet(JNIEnv *env, jobject instance,jstring spath,jstring dpath,jstring start,jboolean getmore,jint num)
{
    string pcmpath1 = jstring2string(env,spath);
    string dstpath = jstring2string(env,dpath);
    string st = jstring2string(env,start);

    VideoJPG mObj;
    mObj.doJpgGet(pcmpath1,dstpath,st,getmore==JNI_TRUE,num);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doJpgToVideo(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    VideoJPG mObj;
    mObj.doJpgToVideo(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doChangeAudioVolume(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    AudioVolume mObj;
    mObj.doChangeAudioVolume(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doChangeAudioVolume2(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    AudioVolume mObj;
    mObj.doChangeAudioVolume2(pcmpath,dpath);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_demo_FFmpegTest_FFmpegTest_doVideoScale(JNIEnv *env, jobject instance,jstring srcpath,jstring dstpath)
{
    string pcmpath = jstring2string(env,srcpath);
    string dpath = jstring2string(env,dstpath);

    FilterVideoScale mObj;
    mObj.doVideoScale(pcmpath,dpath);
}
