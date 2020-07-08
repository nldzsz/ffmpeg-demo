//
//  audio_encode.cpp
//  audio_encode_decode
//
//  Created by apple on 2019/12/3.
//  Copyright © 2019 apple. All rights reserved.
//

#include "audio_encode.hpp"
#include "CLog.h"
#include <math.h>

/**
 *  判断采样格式对于指定的编码器是否支持，如果支持则返回该采样格式；否则返回编码器支持的枚举值最大的采样格式
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
static int select_sample_rate(const AVCodec *codec,int defalt_sample_rate)
{
    const int *p = 0;
    int best_samplerate = 0;
    if (!codec->supported_samplerates) {
        return 44100;
    }
    
    p = codec->supported_samplerates;
    while (*p) {
        if (*p == defalt_sample_rate) {
            return *p;
        }
        
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
static uint64_t select_channel_layout(const AVCodec *codec,uint64_t default_layout)
{
    uint64_t best_ch_layout = AV_CH_LAYOUT_STEREO;
    const uint64_t *p = codec->channel_layouts;
    if (p == NULL) {
        return AV_CH_LAYOUT_STEREO;
    }
    int best_ch_layouts = 0;
    while (*p) {
        int layouts = av_get_channel_layout_nb_channels(*p);
        if (*p == default_layout) {
            return *p;
        }
        
        if (layouts > best_ch_layouts) {
            best_ch_layout = *p;
            best_ch_layouts = layouts;
        }
        p++;
    }
    
    
    return best_ch_layout;
}

AudioEncode::AudioEncode(string spath,string dpath)
:srcpath(spath),dstpath(dpath)
{
    pFormatCtx = NULL;
    pCodecCtx = NULL;
    pSwrCtx = NULL;
}

AudioEncode::~AudioEncode()
{
    
}

void AudioEncode::privateLeaseResources()
{
    if (pFormatCtx) {
        avformat_free_context(pFormatCtx);
        pFormatCtx = NULL;
    }
    if (pCodecCtx) {
        avcodec_free_context(&pCodecCtx);
        pCodecCtx = NULL;
    }
    if (pSwrCtx) {
        swr_free(&pSwrCtx);
        pSwrCtx = NULL;
    }
}

void AudioEncode::doEncode1(AVFormatContext*fmtCtx,AVCodecContext *cCtx,AVPacket *packet,AVFrame *srcFrame,FILE *file)
{
    if (fmtCtx == NULL || cCtx == NULL || packet == NULL) {
        return;
    }
    
    int ret = 0;
    // 开始编码;对于音频编码来说，不需要设置pts的值(但是会出现警告);如果frame 为NULL，则代表将编码缓冲区中的所有剩余数据全部编码完
    ret = avcodec_send_frame(cCtx, srcFrame);
    while (ret >= 0) {
        ret = avcodec_receive_packet(cCtx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { // EAGAIN 有可能是编码器需要先缓冲一部分数据，并不是真正的编码错误
            if (ret == AVERROR_EOF) {
                LOGD("encode error %d",ret);
            }
            return;
        } else if (ret < 0) {   // 产生了真正的编码错误
            return;
        }
        LOGD("packet size %d dts:%d pts:%d duration:%d",packet->size,packet->dts,packet->pts,packet->duration);
        av_write_frame(fmtCtx, packet);
        if (file) {
            fwrite(packet->data, packet->size, 1, file);
        }
        // 每次编码avcodec_receive_packet都会重新为packet分配内存，所以这里用完之后要主动释放
        av_packet_unref(packet);
    }
}

/**
 *  1、一个编码格式可以有多个封装格式，一个封装格式也可以用于多个编码格式。要查询一个编码格式支持的所有封装格式，打开ffmpeg源码通过以下方式查询：
 *      以AAC为例，输入 .audio_codec       = AV_CODEC_ID_AAC  即可查询所有支持AAC的封装格式，对应的文件后缀名为.extensions        =对应的值
 *  2、用AVFormatContext进行封装时，ffmpeg必须编译了对应的封装格式选项才可以，对应的封装格式选项名为.name              =对应的值。即在编译时加上--enable_muxer=该值即可
*/
static string muxer_file_name(CodecFormat format)
{
    string name = "test.aac";
    if (format == CodecFormatMP3) {
        name = "test.mp3";
    } else if (format == CodecFormatAC3) {
        /** 遇到问题：[NULL @ 0x101023a00] Unable to find a suitable output format for 'test.rm'
         *  原因分析：由于没有将ac3 muxer编译进去，从编译产生的config.h文件可以看到#define CONFIG_RM_MUXER 0
         *  解决思路：重新编译即可
         */
        name = "test.ac3";
    }
    
    return name;
}

/**
 *  1、常见音频编码格式介绍，参考博客文档：https://blog.csdn.net/houxiaoni01/article/details/78810674
 *  2、打开ffmpeg的源码通过             .id                    = AV_CODEC_ID_AAC
 *  (这里以AAC编码器为例)查询编码器信息
 *  3、新版本编码器的注册方式(旧版本是通过av_register_xxx以及宏定义添加的)新版本是在.configure之后根据配置自动再libavcodec文件夹下生成一个文件codec_list.c，这个
 *  文件下通过一个静态数组codec_list[]定义了已经编译进去的所有编解码器;参考.configure配置代码 print_enabled_components libavcodec/codec_list.c AVCodec codec_list $CODEC_LIST
 *  4、ffmpeg库源码中可能会出现相同的id对应对个编码库，比如AV_CODEC_ID_AAC就对应libshine和libmp3lame两个库，那么通过ID查找编码器的时候就意味着codec_list[]中索引再
 *  前的编码器库会优先找到(这里要注意)，最好通过名字来查找
 *  5、如果wrapper_name有值，那么说明该编解码器一般为外部库或者系统库，为NULL则说明ffmpeg的自身代码就包含
 */
static AVCodec* select_codec(CodecFormat format)
{
    AVCodec *codec = NULL;
    if (format == CodecFormatAAC) {
        /** 可以通过如下两个方法查找编码器，aac编码器对应的库是fdk_aac
         */
        codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
//        codec = avcodec_find_encoder_by_name("libfdk_aac");
    } else if (format == CodecFormatMP3) {
//        codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
        codec = avcodec_find_encoder_by_name("libmp3lame");
    } else if (format == CodecFormatAC3) {
        codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
    }
    
    return codec;
}

static int select_bit_rate(CodecFormat format)
{
    // 对于不同的编码器最优码率不一样，单位bit/s;对于mp3来说，192kbps可以获得较好的音质效果。
    int bit_rate = 64000;
    if (format == CodecFormatMP3) {
        bit_rate = 192000;
    } else if (format == CodecFormatAC3) {
        bit_rate = 192000;
    }
    
    return bit_rate;
}

void AudioEncode::doEncode(CodecFormat format, bool saveByFile)
{
    if (srcpath.length()==0) {
        LOGD("srcpath not found");
        return;
    }
    if (dstpath.length() == 0) {
        LOGD("dstpath not found");
        return;
    }
    string dstpath1 = dstpath;
    uint64_t src_ch_layout=AV_CH_LAYOUT_STEREO,dst_ch_layout = AV_CH_LAYOUT_STEREO;
    int  src_sample_rate = 44100,dst_sample_rate=44100;
    int  src_nb_samples = 1024;
    int  src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_FLT;// 文件是16位整形类型方式存储的
    enum AVSampleFormat dst_sample_fmt = AV_SAMPLE_FMT_FLT;
    LOGD("srcpath %s dstpath %s \n",srcpath.c_str(),dstpath.c_str());
    
    int ret = 0;
    // 1、查找编码器
    AVCodec *pCodec = select_codec(CodecFormatAAC);
    if (pCodec == NULL) {
        LOGD("pCodec is null \n");
        return;
    }
    
    // 2、创建编码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
        LOGD("pCodecCtx is null \n");
        privateLeaseResources();
        return;
    }
    
    // 3、设置编码参数
    // 对于不同的编码器最优码率不一样，单位bit/s;
    pCodecCtx->bit_rate = select_bit_rate(format);
    // 采样率；每个编码器可能支持多种采样率，默认指定一个采样率(编码器不一定支持默认的)
    pCodecCtx->sample_rate = select_sample_rate(pCodec,dst_sample_rate);
    // 采样格式；每个编码器支持的采样格式不一样，默认指定一个采样格式(编码器不一定支持默认的)
    pCodecCtx->sample_fmt = select_sample_fmt(pCodec, dst_sample_fmt);
    // 声道格式;每个编码器支持的声道格式不一样，默认指定一个声道格式(编码器不一定支持默认的)
    pCodecCtx->channel_layout = select_channel_layout(pCodec,dst_ch_layout);
    // 声道数；声道数和声道格式对应，不懂为什么这里还要重新设置？
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    // 设置时间基，对于音频编码来说 不需要设置此选项
//    pCodecCtx->time_base = AVRational{1,1024};

    /** 4、打开编码器上下文
     * 会对编码器做一些初始化的操作
     */
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0) {
        LOGD("avcodec_open2 fail");
        privateLeaseResources();
        return;
    }
    LOGD("rate %d select fmt %s chl %d frame_size %d",pCodecCtx->sample_rate,av_get_sample_fmt_name(pCodecCtx->sample_fmt),pCodecCtx->channels,pCodecCtx->frame_size);
    
    FILE *file = fopen(srcpath.c_str(), "rb");
    FILE *file1 = NULL;
    if (saveByFile) {
        file1 = fopen(dstpath1.c_str(),"wb+");
    }
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
     *  对于字节对齐方式分配内存大小的总结：类似下面av_samples_alloc_array_and_samples()函数和av_frame_get_buffer()函数的最后一个参数
     *  1、分配的内存总大小为 line_size*声道数，其中line_size >= nb_samples * 每个采样字节数(音频)；
     *  2、如果指定了align 参数，那么line_size的大小为align参数的整数倍，如果为1，则line_size=nb_samples*每个采样字节数
     *  3、当align为0时，自动根据目前cpu架构位数进行分配，但最终不一定是按照32或者64的参数进行分配
     */
    // 5、分配内存块，用于存储未压缩的音频数据
    bool needConvert = false;
    AVPacket    *packet = av_packet_alloc();
    AVFrame     *srcFrame = av_frame_alloc();
    AVFrame     *dstFrame = av_frame_alloc();
    // 每个声道音频的采样数大小
    srcFrame->nb_samples = src_nb_samples;
    dstFrame->nb_samples = pCodecCtx->frame_size;
    // 采样格式
    srcFrame->format = src_sample_fmt;
    dstFrame->format = pCodecCtx->sample_fmt;
    // 声道格式
    srcFrame->channel_layout = src_ch_layout;
    dstFrame->channel_layout = pCodecCtx->channel_layout;
    // 采样率
    srcFrame->sample_rate = src_sample_rate;
    dstFrame->sample_rate = pCodecCtx->sample_rate;
    // 通过以上三个参数即可确定一个AVFrame的大小了，即其中的音频数据的大小；
    // 然后通过此方法分配对应的内存块；第二个参数代表根据cpu的架构自动选择对齐位数，最好填写为0
    ret = av_frame_get_buffer(srcFrame, 0);
    if (ret < 0) {
        LOGD("av_frame_get_buffer fail %d",ret);
        privateLeaseResources();
        return;
    }
    // 让内存块可写，最好调用一下此方法
    ret = av_frame_make_writable(srcFrame);
    if (ret < 0) {
        LOGD("av_frame_make_writable fail %d",ret);
        privateLeaseResources();
        return;
    }
    // 是否需要格式转换
    if (pCodecCtx->sample_fmt != srcFrame->format) {
        needConvert = true;
    }
    if (pCodecCtx->channel_layout != srcFrame->channel_layout) {
        needConvert = true;
    }
    if (pCodecCtx->sample_rate != srcFrame->sample_rate) {
        needConvert = true;
    }
    /** 格式转换
    *  遇到问题：奔溃
    *  原因分析：AVFrame中的音频数据格式和AVCodecContext编码要求的音频数据格式(比如采样格式)不一致
    *  解决方案：编码之前进行格式转换
    */
    if (needConvert) {
        if (pSwrCtx == NULL) {
            pSwrCtx = swr_alloc_set_opts(NULL, pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
                                         srcFrame->channel_layout, (enum AVSampleFormat)srcFrame->format, srcFrame->sample_rate, 0, NULL);
//            pSwrCtx = swr_alloc();
            if (pSwrCtx == NULL) {
                LOGD("swr_alloc_set_opts() fail");
                return;
            }
//            av_opt_set_int(pSwrCtx,"in_channel_layout",srcFrame->channel_layout,AV_OPT_SEARCH_CHILDREN);
//            av_opt_set_sample_fmt(pSwrCtx,"in_sample_fmt",(enum AVSampleFormat)srcFrame->format,AV_OPT_SEARCH_CHILDREN);
//            av_opt_set_int(pSwrCtx,"in_sample_rate",srcFrame->sample_rate,AV_OPT_SEARCH_CHILDREN);
//            av_opt_set_int(pSwrCtx,"out_channel_layout",pCodecCtx->channel_layout,AV_OPT_SEARCH_CHILDREN);
//            av_opt_set_sample_fmt(pSwrCtx,"out_sample_fmt",pCodecCtx->sample_fmt,AV_OPT_SEARCH_CHILDREN);
//            av_opt_set_int(pSwrCtx,"out_sample_rate",pCodecCtx->sample_rate,AV_OPT_SEARCH_CHILDREN);
//
//            ret = swr_init(pSwrCtx);
//            if (ret < 0) {
//                LOGD("swr_init fail");
//                return;
//            }
        }
        
        ret = av_frame_get_buffer(dstFrame,0);
        if (ret < 0) {
            LOGD("av_frame_get_buffer fail %d",ret);
            return;
        }
        ret = av_frame_make_writable(dstFrame);
        if (ret < 0) {
            LOGD("av_frame_make_writable %d",ret);
            return;
        }
    }
    
    /** 6、创建aac格式的封装Muxer上下文环境AVFormatContext上下文环境，对于每个封装器Muxer，它必须包括：
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
    av_dump_format(pFormatCtx, 0, NULL, 1);
    
    /** 遇到问题：编码aac时打印Encoder did not produce proper pts, making some up.警告
     *  分析原因：AVFrame的pts参数没有设置值，设置即可。
     */
    size_t readsize = 0;
    int ptsIndex = 0;
    int require_size = av_samples_get_buffer_size(NULL, src_nb_channels, src_nb_samples, src_sample_fmt, 0);
    while ((readsize = fread(srcFrame->data[0], 1, require_size, file)) > 0) {
        if (needConvert) {
            ret = swr_convert_frame(pSwrCtx,dstFrame,srcFrame);
            if (ret < 0) {
                LOGD("swr_convert_frame fail %d",ret);
                continue;
            }
            dstFrame->pts = ptsIndex;
            doEncode1(pFormatCtx, pCodecCtx, packet, dstFrame,file1);
        } else {
            srcFrame->pts = ptsIndex;
            doEncode1(pFormatCtx,pCodecCtx,packet,srcFrame,file1);
        }
        ptsIndex++;
        
//        // 先将packet格式转换成planner格式，每个采样是4个字节，所以这里可以转换成32无符号整数这样可以一次性读取4个字节，代码更加简洁
//        /** 遇到问题：播放声音变质了
//         *  解决方案：是因为fdst[j][i] = fsrc[0][i*src_nb_channels+j];代码之前为fdst[j][i] = fsrc[0][i*+j];导致数据错乱更正即可
//        */
//        uint32_t **fdst = (uint32_t**)aframe->data;
//        uint32_t **fsrc = (uint32_t**)src_data;
//        for (int i=0; i<src_nb_samples ; i++) {
//            for (int j=0; j<src_nb_channels; j++) {
//                if (av_sample_fmt_is_planar((AVSampleFormat)aframe->format)) {
//                    fdst[j][i] = fsrc[0][i*src_nb_channels+j];
////                    LOGD("add[%d] %p",j,fdst[j][i]);
////                    printUint32toHex(fdst[j][i]);
//                } else {
//                    fdst[0][i*src_nb_channels+j] = fsrc[0][i*src_nb_channels+j];
//                }
//            }
//        }
        
    }
    // 如果有进行格式转换 则将缓冲区中的内容全部取出来
    if (needConvert) {
        while (swr_convert_frame(pSwrCtx,dstFrame,NULL) == 0) {
            if (dstFrame->nb_samples > 0) {
                LOGD("清除剩余的 %d",dstFrame->nb_samples);
                dstFrame->pts = ptsIndex;
                doEncode1(pFormatCtx, pCodecCtx, packet, dstFrame,file1);
            } else {
                break;
            }
        }
    }
    // 传入NULL则将剩余未编码完的数据全部编码
    doEncode1(pFormatCtx,pCodecCtx,packet,NULL,file1);
    
    // 写入收尾信息，必须要与，否则文件无法播放
    av_write_trailer(pFormatCtx);
    fclose(file);
    if (file1) {
        fclose(file1);
    }
    av_packet_free(&packet);
    av_frame_unref(srcFrame);
    av_frame_unref(dstFrame);
    
    privateLeaseResources();
}
