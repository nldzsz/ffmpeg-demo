//
//  audio_encode.hpp
//  audio_encode_decode
//
//  Created by apple on 2019/12/3.
//  Copyright © 2019 apple. All rights reserved.
//

#ifndef audio_encode_hpp
#define audio_encode_hpp

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}
#include <stdio.h>
#include <string>

/** zsz:todo 如果出现类似 dyld: Library not loaded: /usr/local/opt/ffmpeg/lib/libavcodec.58.dylib这样的错误
 *  则进入target-->signing&capabilities-->将选项Disable Library Validation 打上钩
 */
using namespace std;
typedef enum {
    CodecFormatAAC,
    CodecFormatMP3,
    CodecFormatAC3,
} CodecFormat;

class AudioEncode
{
public:
    AudioEncode(string spath,string dpath);
    ~AudioEncode();
    /** aac有两种分装格式，ADIF和ADTS，比较常用的是ADTS。FFMpeg进行aac编码后的数据就是ADTS的格式数据。这个数据直接写入文件即可播放。
     *  doEncode默认通过FFMpeg库提供的AVFormatContext写入数据，如果saveByFile为true，还同时直接将编码后的aac数据由File提供接口写入文件
     */
    void doEncode(CodecFormat format=CodecFormatAAC, bool saveByFile = false);
    
private:
    string srcpath,dstpath;
    AVCodecContext  *pCodecCtx;
    AVFormatContext *pFormatCtx;
    SwrContext      *pSwrCtx;
    
    void privateLeaseResources();
    void doEncode1(AVFormatContext*fmtCtx,AVCodecContext *cCtx,AVPacket *packet,AVFrame *frame,FILE *file);
};

#endif /* audio_encode_hpp */
