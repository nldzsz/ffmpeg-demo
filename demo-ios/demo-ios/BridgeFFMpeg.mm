//
//  BridgeFFMpeg.m
//  demo-ios
//
//  Created by apple on 2020/7/13.
//  Copyright © 2020 apple. All rights reserved.
//

#import "BridgeFFMpeg.h"
#include <map>
#include <string>

// for log
extern "C" {
#include "Common.h"
#include "libavcodec/avcodec.h"
#include "libavcodec/jni.h"
}
#include <string>
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

@implementation BridgeFFMpeg
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

+(void)testFFmpeg
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
std::string jstring2string(NSString*jStr)
{
    const char *cstr = [jStr UTF8String];
    std::string str = std::string(cstr);
    return str;
}

+(void)doEncodecPcm2aac:(NSString*)src dst:(NSString*)dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    AudioEncode aEncode(pcmpath,dpath);
    aEncode.doEncode(CodecFormatAAC,false);
}
+(void)doResample:(NSString*)src dst:(NSString*)dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    AudioResample aResample;
    aResample.doResample(pcmpath,dpath);
}
+(void)doResampleAVFrame:(NSString*)srcpath dst:(NSString*)dstpath
{
    string pcmpath = jstring2string(srcpath);
    string dpath = jstring2string(dstpath);

    AudioResample aResample;
    aResample.doResampleAVFrame(pcmpath,dpath);
}
+(void)doScale:(NSString*)src dst:(NSString*)dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);
    VideoScale scale;
    scale.doScale(pcmpath,dpath);
}
+(void)doDemuxer:(NSString*)src
{
    string pcmpath = jstring2string(src);

    Demuxer demuxer;
    demuxer.doDemuxer(pcmpath);
}
+(void)doMuxerTwoFile:(NSString*)audio_src video_src:(NSString*)video_src dst:(NSString*) dst;
{
    string audio_path = jstring2string(audio_src);
    string video_vpath = jstring2string(video_src);
    string dpath = jstring2string(dst);

    Muxer muxer;
    muxer.doMuxerTwoFile(audio_path,video_vpath,dpath);
}
+(void)doReMuxer:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    Muxer muxer;
    muxer.doReMuxer(pcmpath,dpath);
}
+(void)doSoftDecode:(NSString*)src
{
    string pcmpath = jstring2string(src);
    SoftEnDecoder decoder;
    decoder.doDecode(pcmpath);
}
+(void)doSoftDecode2:(NSString*)src
{
    string pcmpath = jstring2string(src);

    SoftEnDecoder decoder;
    decoder.doDecode2(pcmpath);
}
+(void)doSoftEncode:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    SoftEnDecoder decoder;
    decoder.doEncode(pcmpath,dpath);
}
+(void)doHardDecode:(NSString*) src
{
    string pcmpath = jstring2string(src);

    HardEnDecoder decoder;
    decoder.doDecode(pcmpath,HardTypeVideoToolbox);
}
+(void)doHardEncode:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    HardEnDecoder decoder;
    decoder.doEncode(pcmpath,dpath,HardTypeVideoToolbox);
}
+(void)doReMuxerWithStream:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    Muxer demuxer;
    demuxer.doReMuxerWithStream(pcmpath,dpath);
}
+(void)doExtensionTranscode:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    Transcode codec;
    codec.doExtensionTranscode(pcmpath,dpath);
}
+(void)doTranscode:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    Transcode codec;
    codec.doTranscode(pcmpath,dpath);
}
+(void)doEncodeMuxer:(NSString*)dst
{
    string dpath = jstring2string(dst);

    EncodeMuxer muxer;
    muxer.doEncodeMuxer(dpath);
}
+(void)doCut:(NSString*)src dst:(NSString*)dst start:(NSString*)start du:(int)du
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);
    string st = jstring2string(start);

    Cut cutObj;
    cutObj.doCut(pcmpath,dpath,st,du);
}
+(void)MergeTwo:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst
{
    string pcmpath1 = jstring2string(src1);
    string pcmpath2 = jstring2string(src2);
    string dpath = jstring2string(dst);
    Merge mObj;
    mObj.MergeTwo(pcmpath1,pcmpath2,dpath);
}
+(void)MergeFiles:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst
{
    string pcmpath1 = jstring2string(src1);
    string pcmpath2 = jstring2string(src2);
    string dpath = jstring2string(dst);

    Merge mObj;
    mObj.MergeFiles(pcmpath1,pcmpath2,dpath);
}
+(void)addMusic:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst start:(NSString*)st
{
    string pcmpath1 = jstring2string(src1);
    string pcmpath2 = jstring2string(src2);
    string dpath = jstring2string(dst);
    string start = jstring2string(st);

    Merge mObj;
    mObj.addMusic(pcmpath1,pcmpath2,dpath,start);
}
+(void)doJpgGet:(NSString*)src dst:(NSString*)dst start:(NSString*)start getmore:(BOOL)getmore num:(int)num;
{
    string pcmpath1 = jstring2string(src);
    string dstpath = jstring2string(dst);
    string st = jstring2string(start);

    VideoJPG mObj;
    mObj.doJpgGet(pcmpath1,dstpath,st,getmore==YES,num);
}
+(void)doJpgToVideo:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    VideoJPG mObj;
    mObj.doJpgToVideo(pcmpath,dpath);
}
+(void)doChangeAudioVolume:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    AudioVolume mObj;
    mObj.doChangeAudioVolume(pcmpath,dpath);
}
+(void)doChangeAudioVolume2:(NSString*)src dst:(NSString*) dst
{
    string pcmpath = jstring2string(src);
    string dpath = jstring2string(dst);

    AudioVolume mObj;
    mObj.doChangeAudioVolume2(pcmpath,dpath);
}
+(void)doAcrossfade:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*) dst duration:(int)duration
{
    string pcmpath1 = jstring2string(src1);
    string pcmpath2 = jstring2string(src2);
    string dpath = jstring2string(dst);

    AudioAcrossfade mObj;
    mObj.doAcrossfade(pcmpath1, pcmpath2, dpath,duration);
}
+ (void)doVideoScale:(NSString *)src dst:(NSString *)dst
{
    string pcmpath1 = jstring2string(src);
    string dpath = jstring2string(dst);

    VideoScale mObj;
    mObj.doScale(pcmpath1, dpath);
}
+(void)addSubtitleStream:(NSString*)vpath src2:(NSString*)spath dst:(NSString*)dst
{
    string pcmpath1 = jstring2string(vpath);
    string pcmpath2 = jstring2string(spath);
    string dpath = jstring2string(dst);

    Subtitles mObj;
    mObj.addSubtitleStream(pcmpath1, pcmpath2,dpath);
}
+(void)configConfpath:(NSString*)confpath fontsPath:(NSString*)fontspath withFonts:(NSDictionary*)fontsDic
{
    if ([[NSFileManager defaultManager] fileExistsAtPath:confpath]) {
        [[NSFileManager defaultManager] removeItemAtPath:confpath error:nil];
    }
    [[NSFileManager defaultManager] createFileAtPath:confpath contents:nil attributes:nil];
    
    string pcmpath1 = jstring2string(confpath);
    string pcmpath2 = jstring2string(fontspath);

    Subtitles mObj;
    map<std::string,std::string>fonts;
    for (NSString *font in fontsDic.allKeys) {
        NSString *mapedfont = fontsDic[font];
        std::string cxxFont([font UTF8String]);
        std::string cxxMapedfont([mapedfont UTF8String]);
        fonts[cxxFont] = cxxMapedfont;
    }
    mObj.configConfpath(pcmpath1, pcmpath2, fonts);
}
+(void)addSubtitlesForVideo:(NSString*)vpath src2:(NSString*)spath dst:(NSString*)dst confdpath:(NSString*)confdpath
{
    string pcmpath1 = jstring2string(vpath);
    string pcmpath2 = jstring2string(spath);
    string dpath = jstring2string(dst);
    string cpath = jstring2string(confdpath);

    Subtitles mObj;
    mObj.addSubtitlesForVideo(pcmpath1, pcmpath2,dpath,cpath);
}
@end
