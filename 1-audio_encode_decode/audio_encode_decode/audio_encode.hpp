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
#include <libavutil/error.h>
}
#include <stdio.h>
#include <string>

/** zsz:todo 如果出现类似 dyld: Library not loaded: /usr/local/opt/ffmpeg/lib/libavcodec.58.dylib这样的错误
 *  则进入target-->signing&capabilities-->将选项Disable Library Validation 打上钩
 */
using namespace std;

class AudioEncode
{
public:
    AudioEncode();
    ~AudioEncode();
    /** aac有两种分装格式，ADIF和ADTS，比较常用的是ADTS。FFMpeg再进行aac编码后还需要将编码后的音频封装为aac格式，
     *  这个过程称为aac muxer，有两种方式。方式一：通过FFMpeg库提供的接口封装；方式二：手动封装
     *  doEncode_flt32_le2_to_aac1通过FFMpeg库提供的接口封装，doEncode_flt32_le2_to_aac2手动封装
     */
    void doEncode_flt32_le2_to_aac1(string &pcmpath,string &dstpath);
    void doEncode_flt32_le2_to_aac2(string &pcmpath,string &dstpath);
    
private:
    AVCodec         *pCodec;
    AVCodecContext  *pCodecCtx;
    AVFormatContext *pFormatCtx;
    
    void privateLeaseResources();
    void doEncode1(AVFormatContext*fmtCtx,AVCodecContext *cCtx,AVPacket *packet,AVFrame *frame);
    void doEncode2(AVCodecContext *cCtx,AVPacket *packet,AVFrame *frame,FILE *file);
};

#endif /* audio_encode_hpp */
