//
//  audio_encode.cpp
//  audio_encode_decode
//
//  Created by apple on 2019/12/3.
//  Copyright © 2019 apple. All rights reserved.
//

#include "audio_encode.hpp"
#include "cppcommon/CLog.h"
#include <math.h>

/**
 *  判断采样格式对于指定的编码器是否支持，如果支持则返回该采样格式；否则返回编码器支持的采样格式
 */
static enum AVSampleFormat select_sample_fmt(const AVCodec *codec,enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;
    enum AVSampleFormat rfmt = AV_SAMPLE_FMT_NONE;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt) {
            return sample_fmt;
        }
        if (rfmt == AV_SAMPLE_FMT_NONE) {
            rfmt = *p;
        }
        p++;
    }
    
    return rfmt;
}

/**
 *  返回指定编码器接近尽量接近44100的采样率
 */
static int select_sample_rate(const AVCodec *codec)
{
    const int *p = 0;
    int best_samplerate = 0;
    if (!codec->supported_samplerates) {
        return 44100;
    }
    
    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate)) {
            best_samplerate = *p;
        }
        
        p++;
    }
    
    return best_samplerate;
}

/** 返回编码器支持的声道格式中声道数最多的声道格式
 *  声道格式和声道数一一对应
 */
static uint64_t select_channel_layout(const AVCodec *codec)
{
    uint64_t best_ch_layout = 0;
    const uint64_t *p = codec->channel_layouts;
    if (p == NULL) {
        return AV_CH_LAYOUT_STEREO;
    }
    int best_ch_layouts = 0;
    while (*p) {
        int layouts = av_get_channel_layout_nb_channels(*p);
        if (layouts > best_ch_layouts) {
            best_ch_layout = *p;
            best_ch_layouts = layouts;
        }
        p++;
    }
    
    
    return best_ch_layout;
}

AudioEncode::AudioEncode()
{
    
}

AudioEncode::~AudioEncode()
{
    
}

void AudioEncode::privateLeaseResources()
{
    if (pFormatCtx) {
        avformat_close_input(&pFormatCtx);
        pFormatCtx = NULL;
    }
    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
        pCodecCtx = NULL;
    }
}

void AudioEncode::doEncode2(AVCodecContext *cCtx,AVPacket *packet,AVFrame *frame,FILE *file)
{
    if (cCtx == NULL || packet == NULL || file == NULL) {
        return;
    }
    
    // 组装ADTS
    char adts[7];
    int profile = 2;        //AAC LC    编码级别
    int freqIdx = 4;        //44.1KHz   采样率
    int chanCfg = 2;        //MPEG-4 Audio Channel Configuration. 1 Channel front-center    声道数
    adts[0] = (char)0xFF;  // 11111111     = syncword
    adts[1] = (char)0xF1;  // 1111 1 00 1  = syncword MPEG-2 Layer CRC
    adts[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    adts[6] = (char)0xFC;
    
    int ret = 0;
    // 开始编码;对于音频编码来说，不需要设置pts的值;如果frame 为NULL，则代表将编码缓冲区中的所有剩余数据全部编码完
    ret = avcodec_send_frame(cCtx, frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(cCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { // EAGAIN 有可能是编码器需要先缓冲一部分数据，并不是真正的编码错误
            LOGD("encode error %d",ret);
            return;
        } else if (ret < 0) {   // 产生了真正的编码错误
            return;
        }
        LOGD("packet size %d dts:%d pts:%d duration:%d",packet->size,packet->dts,packet->pts,packet->duration);
        
        adts[3] = (char)(((chanCfg & 3) << 6) + ((7 + packet->size) >> 11));
        adts[4] = (char)(((7 + packet->size) & 0x7FF) >> 3);
        adts[5] = (char)((((7 + packet->size) & 7) << 5) + 0x1F);
        fwrite(adts, 7, 1, file);
        fwrite(packet->data, 1, packet->size, file);

        // 每次编码avcodec_receive_packet都会重新为packet分配内存，所以这里用完之后要主动释放
        av_packet_unref(packet);
    }
}

void AudioEncode::doEncode1(AVFormatContext*fmtCtx,AVCodecContext *cCtx,AVPacket *packet,AVFrame *frame)
{
    if (fmtCtx == NULL || cCtx == NULL || packet == NULL) {
        return;
    }
    
    int ret = 0;
    // 开始编码;对于音频编码来说，不需要设置pts的值;如果frame 为NULL，则代表将编码缓冲区中的所有剩余数据全部编码完
    ret = avcodec_send_frame(cCtx, frame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(cCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { // EAGAIN 有可能是编码器需要先缓冲一部分数据，并不是真正的编码错误
            LOGD("encode error %d",ret);
            return;
        } else if (ret < 0) {   // 产生了真正的编码错误
            return;
        }
        LOGD("packet size %d dts:%d pts:%d duration:%d",packet->size,packet->dts,packet->pts,packet->duration);
        av_write_frame(fmtCtx, packet);
        // 每次编码avcodec_receive_packet都会重新为packet分配内存，所以这里用完之后要主动释放
        av_packet_unref(packet);
    }
}

void AudioEncode::doEncode_flt32_le2_to_aac1(string &pcmpath, string &dstpath)
{
    if (pcmpath.length() == 0) {
        LOGD("pcmpath lengh %ld empty %d\n",pcmpath.length(),pcmpath.empty());
        return;
    }
    if (dstpath.length() == 0) {
        LOGD("dstpath lengh %ld empty %d\n",dstpath.length(),dstpath.empty());
        return;
    }
    LOGD("srcpath %s dstpath %s \n",pcmpath.c_str(),dstpath.c_str());
    
    int ret = 0;
    /** 1、查找aac编码器
     *  如果通过AV_CODEC_ID_AAC编码器方式查找编码器或者解码器则是使用的ffmpeg内置的aac编解码库
     *  如果通过avcodec_find_encoder_by_name()或者avcodec_find_decoder_by_name()方式获取的编解码器
     *  则是使用fdk_aac编解码库(前提这个库得编译到ffmpeg库中)
     */
    pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
//    pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (pCodec == NULL) {
        LOGD("pCodec is null \n");
        return;
    }
    
    /** 2、创建编码器上下文
     */
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        LOGD("pCodecCtx is null \n");
        privateLeaseResources();
        return;
    }
    
    /** 3、设置编码参数
     */
    // 对于不同的编码器最优码率不一样，单位bit/s;对于mp3来说，192kbps可以获得较好的音质效果。
    pCodecCtx->bit_rate = 64000;
    // 采样率；每个编码器可能支持多种采样率，这里选择能支持的最优采样率
    pCodecCtx->sample_rate = select_sample_rate(pCodec);
    // 采样格式；这里默认选择的是16位整数 packet方式存储数据的格式
    pCodecCtx->sample_fmt = select_sample_fmt(pCodec, AV_SAMPLE_FMT_S16);
    // 声道格式;每个编码器都可能支持多个声道格式，这里选择的是声道数最多的声道格式
    pCodecCtx->channel_layout = select_channel_layout(pCodec);
    // 声道数；声道数和声道格式对应，不懂为什么这里还要重新设置？
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    LOGD("rate %d select fmt %s chl %d",pCodecCtx->sample_rate,av_get_sample_fmt_name(pCodecCtx->sample_fmt),pCodecCtx->channels);
    
    /** 4、打开编码器上下文
     * 会对编码器做一些初始化的操作
     */
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0) {
        LOGD("avcodec_open2 fail");
        privateLeaseResources();
        return;
    }
    
    // 5、分配内存块，用于存储未压缩的音频数据
    AVFrame     *aframe = av_frame_alloc();
    // 每个声道音频的采样数大小；frame_size在 avcodec_open2()调用之后被初始化
    aframe->nb_samples = pCodecCtx->frame_size;
    // 采样格式
    aframe->format = pCodecCtx->sample_fmt;
    // 声道格式
    aframe->channel_layout = pCodecCtx->channel_layout;
    // 通过以上三个参数即可确定一个AVFrame的大小了，即其中的音频数据的大小；
    // 然后通过此方法分配对应的内存块；第二个参数代表根据cpu的架构自动选择对齐位数，最好填写为0
    ret = av_frame_get_buffer(aframe, 0);
    if (ret < 0) {
        LOGD("av_frame_get_buffer fail %d",ret);
        privateLeaseResources();
        return;
    }
    // 让内存块可写，最好调用一下此方法
    ret = av_frame_make_writable(aframe);
    if (ret < 0) {
        LOGD("av_frame_make_writable fail %d",ret);
        privateLeaseResources();
        return;
    }
    
    
    FILE *file = fopen(pcmpath.c_str(), "rb");
    if (file == NULL) {
        LOGD("fopen fail");
        privateLeaseResources();
        return;
    }
    
    /** AVFrame中音频存储数据的方式：
     *  planner方式：每个声道的数据分别存储在data[0],data[1]...中
     *  packet方式：顺序存储方式，每个声道的采样数据都存储在data[0]中，在内存中依次存储，以双声道为例，比如LRLRLRLR......，其中
     *  L代表左声道数据，R代表右声道数据
     *  具体的存储方式由AVFrame的format属性决定，参考AVSampleFormat枚举，带P的为planner方式，否则packet方式
     *
     *  PCM文件中音频数据的存储方式：
     *  以双声道为例，一般都是按照LRLR.....的方式存储，也就是packet方式。所以从pcm文件中读取数据到AVFrame中时，要注意存储方式是否对应。
     */
    // 用于存储编码后的数据
    AVPacket    *packet = av_packet_alloc();
    // 首先计算每次从文件中要读取的字节数大小，这里根据声道数，采样数，采样格式来计算。
    uint8_t **src_data;
    int src_linesize;
    int src_nb_channels = aframe->channels;
    int src_nb_samples = aframe->nb_samples;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT;// 文件就是浮点类型方式存储的
    // 返回成功分配的字节数
    int require_size = av_samples_alloc_array_and_samples(&src_data, &src_linesize,src_nb_channels, src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        LOGD("av_samples_alloc_array_and_samples fail");
        privateLeaseResources();
        return;
    }
    
    /** 创建aac格式的封装Muxer上下文环境AVFormatContext上下文环境，对于每个封装器Muxer，它必须包括：
     *  音频流参数信息，由AVStream表达包含的封装参数信息
     */
    // 根据输出文件后缀猜测AVFormatContext上下文。
    ret = avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, dstpath.c_str());
    if (ret < 0) {
        LOGD("avformat_alloc_output_context2 fail %d",ret);
        privateLeaseResources();
        return;
    }
    // 添加音频流参数信息
    AVStream *ostream = avformat_new_stream(pFormatCtx, NULL);
    if (!ostream) {
        LOGD("avformat_new_stream fail");
        privateLeaseResources();
        return;
    }
    
    // 打开上下文用于准备写入数据
    ret = avio_open(&pFormatCtx->pb, dstpath.c_str(), AVIO_FLAG_READ_WRITE);
    if (ret < 0) {
        LOGD("avio_open fail %d",ret);
        privateLeaseResources();
        return;
    }
    /** 写入头文件信息   重要：将编码器信息拷贝到AVFormatContext的对应的AVSream流中
     *  遇到问题：返回错误-22，提示Only AAC streams can be muxed by the ADTS muxer
     *  解决方案：通过avcodec_parameters_from_context方法将编码器信息拷贝到输出流的AVStream中codecpar参数
    */
    avcodec_parameters_from_context(ostream->codecpar, pCodecCtx);
    ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        LOGD("avformat_write_header %d",ret);
        privateLeaseResources();
        return;
    }
    
    size_t readsize = 0;
    while ((readsize = fread(src_data[0], 1, require_size, file)) > 0) {
        
        // 先将packet格式转换成planner格式，每个采样是4个字节，所以这里可以转换成32无符号整数这样可以一次性读取4个字节，代码更加简洁
        /** 遇到问题：播放声音变质了，
         *  解决方案：是因为fdst[j][i] = fsrc[0][i*src_nb_channels+j];代码之前为fdst[j][i] = fsrc[0][i*+j];导致数据错乱更正即可
        */
        uint32_t **fdst = (uint32_t**)aframe->data;
        uint32_t **fsrc = (uint32_t**)src_data;
        for (int i=0; i<src_nb_samples; i++) {
            for (int j=0; j<src_nb_channels; j++) {
                fdst[j][i] = fsrc[0][i*src_nb_channels+j];
            }
        }
        
        doEncode1(pFormatCtx,pCodecCtx,packet,aframe);
    }
    // 传入NULL则将剩余未编码完的数据全部编码
    doEncode1(pFormatCtx,pCodecCtx,packet,NULL);
    
    // 写入收尾信息，必须要与，否则文件无法播放
    av_write_trailer(pFormatCtx);
    if (src_data) {
        free(src_data);
    }
    fclose(file);
    av_packet_free(&packet);
    av_frame_free(&aframe);
    
    privateLeaseResources();
}

void AudioEncode::doEncode_flt32_le2_to_aac2(string &pcmpath, string &dstpath)
{
    if (pcmpath.length() == 0) {
        LOGD("pcmpath lengh %ld empty %d\n",pcmpath.length(),pcmpath.empty());
        return;
    }
        
    if (dstpath.length() == 0) {
        LOGD("dstpath lengh %ld empty %d\n",dstpath.length(),dstpath.empty());
        return;
    }
    LOGD("srcpath %s dstpath %s \n",pcmpath.c_str(),dstpath.c_str());
    int ret = 0;
    /** 查找aac编码器
     *  如果通过AV_CODEC_ID_AAC编码器方式查找编码器或者解码器则是使用的ffmpeg内置的aac编解码库
     *  如果通过avcodec_find_encoder_by_name()或者avcodec_find_decoder_by_name()方式获取的编解码器
     *  则是使用fdk_aac编解码库(前提这个库得编译到ffmpeg库中)
     */
    pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
//    pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (pCodec == NULL) {
        LOGD("pCodec is null \n");
        return;
    }
    
    /** 2、创建编码上下文
     */
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        LOGD("pCodecCtx is null \n");
        privateLeaseResources();
        return;
    }
    
    /** 3、设置编码参数
     */
    // 对于不同的编码器最优码率不一样，单位bit/s;对于mp3来说，192kbps可以获得较好的音质效果。
    pCodecCtx->bit_rate = 64000;
    // 采样率；每个编码器能支持多种采样率，这里选择能支持的最优采样率
    pCodecCtx->sample_rate = select_sample_rate(pCodec);
    // 采样格式；这里默认选择的是16位整数 packet方式存储数据的格式
    pCodecCtx->sample_fmt = select_sample_fmt(pCodec, AV_SAMPLE_FMT_S16);
    // 声道格式;每个编码器都可能支持多个声道格式，这里选择的是声道数最多的声道格式
    pCodecCtx->channel_layout = select_channel_layout(pCodec);
    // 声道数；声道数和声道格式对应，不懂为什么这里还要重新设置？
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    
    LOGD("rate %d select fmt %s chl %d",pCodecCtx->sample_rate,av_get_sample_fmt_name(pCodecCtx->sample_fmt),pCodecCtx->channels);
    
    /** 4、打开编码器上下文
     * 会对编码器做一些初始化的操作
     */
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0) {
        LOGD("avcodec_open2 fail");
        privateLeaseResources();
        return;
    }
    
    // 5、为原始音频数据分配内存块，用于存储原始音频数据
    AVFrame     *aframe = av_frame_alloc();
    // 每个声道中装载的采样数；frame_size在 avcodec_open2()调用之后被初始化
    aframe->nb_samples = pCodecCtx->frame_size;
    // 采样格式
    aframe->format = pCodecCtx->sample_fmt;
    // 声道格式
    aframe->channel_layout = pCodecCtx->channel_layout;
    aframe->channels = av_get_channel_layout_nb_channels(aframe->channel_layout);
    // 通过以上三个参数即可确定一个AVFrame的大小了，即其中装载的音频数据的大小，返回大小size
    int size = av_samples_get_buffer_size(NULL, aframe->channels, aframe->nb_samples, pCodecCtx->sample_fmt, 0);
    // 此方法将aframe的linesize成员以及extended_data分配数据，然后将data成员指向第四个参数所指向的内存块，如果第四个参数为NULL，则data成员也为NULL
    uint8_t *frame_buf = (uint8_t*)av_mallocz(size);
    avcodec_fill_audio_frame(aframe, aframe->channels, pCodecCtx->sample_fmt, frame_buf, size, 0);
    
    FILE *file = fopen(pcmpath.c_str(), "rb");
    FILE *dstfile = fopen(dstpath.c_str(), "wb");
    if (file == NULL || dstfile == NULL) {
        LOGD("fopen fail");
        privateLeaseResources();
        return;
    }
    
    /** AVFrame中音频存储数据的方式：
     *  planner方式：每个声道的数据分别存储在data[0],data[1]...中
     *  packet方式：顺序存储方式，每个声道的采样数据都存储在data[0]中，在内存中依次存储，以双声道为例，比如LRLRLRLR......，其中
     *  L代表左声道数据，R代表右声道数据
     *  具体的存储方式由AVFrame的format属性决定，参考AVSampleFormat枚举，带P的为planner方式，否则packet方式
     *
     *  PCM文件中音频数据的存储方式：
     *  以双声道为例，一般都是按照LRLR.....的方式存储，也就是packet方式。所以从pcm文件中读取数据到AVFrame中时，要注意存储方式是否对应。
     */
    // 用于存储编码后的数据
    AVPacket    *packet = av_packet_alloc();
    // 首先计算每次从文件中要读取的字节数大小，这里根据声道数，采样数，采样格式来计算。
    uint8_t **src_data;
    int src_linesize;
    int src_nb_channels = aframe->channels;
    int src_nb_samples = aframe->nb_samples;
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT;// 文件就是浮点类型方式存储的
    // 返回成功分配的字节数
    int require_size = av_samples_alloc_array_and_samples(&src_data, &src_linesize,src_nb_channels, src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        LOGD("av_samples_alloc_array_and_samples fail");
        privateLeaseResources();
        return;
    }
    
    // 根据输出文件后缀猜测AVFormatContext上下文。
    ret = avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, dstpath.c_str());
    if (ret < 0) {
        LOGD("avformat_alloc_output_context2 fail %d",ret);
        privateLeaseResources();
        return;
    }
    // 添加一个流
    AVStream *ostream = avformat_new_stream(pFormatCtx, NULL);
    if (!ostream) {
        LOGD("avformat_new_stream fail");
        privateLeaseResources();
        return;
    }
    
    // 打开上下文用于准备写入数据
    ret = avio_open(&pFormatCtx->pb, dstpath.c_str(), AVIO_FLAG_READ_WRITE);
    if (ret < 0) {
        LOGD("avio_open fail %d",ret);
        privateLeaseResources();
        return;
    }
    /** 写入头文件信息   重要：将编码器信息拷贝到AVFormatContext的对应的AVSream流中
     *  遇到问题：返回错误-22，提示Only AAC streams can be muxed by the ADTS muxer
     *  解决方案：通过avcodec_parameters_from_context方法将编码器信息拷贝到输出流的AVStream中codecpar参数
    */
    avcodec_parameters_from_context(ostream->codecpar, pCodecCtx);
    ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        LOGD("avformat_write_header %d",ret);
        privateLeaseResources();
        return;
    }
    
    
    size_t readsize = 0;
    while ((readsize = fread(src_data[0], 1, require_size, file)) > 0) {
        
        // 先将packet格式转换成planner格式，每个采样是4个字节，所以这里可以转换成32无符号整数这样可以一次性读取4个字节，代码更加简洁
        /** 遇到问题：播放声音变质了，
         *  解决方案：是因为fdst[j][i] = fsrc[0][i*src_nb_channels+j];代码之前为fdst[j][i] = fsrc[0][i*+j];导致数据错乱更正即可
        */
        uint32_t **fdst = (uint32_t**)aframe->data;
        uint32_t **fsrc = (uint32_t**)src_data;
        for (int i=0; i<src_nb_samples ; i++) {
            for (int j=0; j<src_nb_channels; j++) {
                fdst[j][i] = fsrc[0][i*src_nb_channels+j];
            }
        }
        
        doEncode2(pCodecCtx,packet,aframe,dstfile);
    }
    doEncode2(pCodecCtx,packet,NULL,dstfile);
    
    fclose(dstfile);
    fclose(file);
    privateLeaseResources();
}
