//
//  VideoJpg.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/20.
//  Copyright © 2020 apple. All rights reserved.
//

#include "VideoJpg.hpp"

VideoJPG::VideoJPG()
{
    sws_ctx = NULL;
    de_frame = NULL;
    en_frame = NULL;
    in_fmt = NULL;
    ou_fmt = NULL;
    de_ctx = NULL;
    en_ctx = NULL;
}

VideoJPG::~VideoJPG()
{
    
}

void VideoJPG::releaseSources()
{
    if (in_fmt) {
        avformat_close_input(&in_fmt);
        in_fmt = NULL;
    }
    if (ou_fmt) {
        avformat_free_context(ou_fmt);
        ou_fmt = NULL;
    }
    
    if (en_frame) {
        av_frame_unref(en_frame);
        en_frame = NULL;
    }
    
    if (de_frame) {
        av_frame_unref(de_frame);
        de_frame = NULL;
    }
    
    if (en_ctx) {
        avcodec_free_context(&en_ctx);
        en_ctx = NULL;
    }
    
    if (de_ctx) {
        avcodec_free_context(&de_ctx);
        de_ctx = NULL;
    }
}

/** 写入jpg说明：
 *  1、ffmpeg对将像素数据写入到JPG图片中也封装到了avformat_xxx系列接口中，它的使用流程和封装视频数据到mp4文件一模一样
 *  只不过一个JPG文件中只包含了一帧视频数据而已;
 *  2、ffmpeg对JPG文件的封装支持模式匹配，即如果想要将多张图片写入到多张jpg中只需要文件名包含百分号即可，例如 name%3d.jpg，那么在每一次调用av_write_frame()
 *  函数写入视频数据到jpg图片时都会生成一张jpg图片。这样做的好处是不需要每一张要写入的jpg文件都创建一个AVFormatContext与之对应。其它流程和写入一张jpg一样，具体
 *  参考如下示例：
 *  3、jpg对应的封装器为ff_image2_muxer，对应的编码器为ff_mjpeg_encoder
 */
#define Get_More 1      // 1代表使用模式匹配，一次可以写入多张jpg图片。0代表一次写入1张图片
void VideoJPG::doJpgGet(string srcPath,string dstPath,bool getMore,int num)
{
    if (!getMore) {
        num = 1;
    }
    int video_index = -1;
    
    // 要截取的时刻
    string start = "00:00:05";
    int64_t start_pts = stoi(start.substr(0,2));
    start_pts += stoi(start.substr(3,2));
    start_pts += stoi(start.substr(6,2));
    
    if (avformat_open_input(&in_fmt,srcPath.c_str(),NULL,NULL) < 0) {
        LOGD("avformat_open_input fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt, NULL) < 0) {
        LOGD("avformat_find_stream_info fail");
        return;
    }
    
    // 遍历出视频索引
    for (int i = 0; i<in_fmt->nb_streams; i++) {
        AVStream *stream = in_fmt->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {   // 说明是视频
            video_index = i;
            // 初始化解码器用于解码
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            de_ctx = avcodec_alloc_context3(codec);
            if (!de_ctx) {
                LOGD("video avcodec_alloc_context3 fail");
                releaseSources();
                return;
            }
            
            // 设置解码参数，这里直接从源视频流中拷贝
            if (avcodec_parameters_to_context(de_ctx, stream->codecpar) < 0) {
                LOGD("video avcodec_parameters_to_context");
                releaseSources();
                return;
            }
            
            // 初始化解码器上下文
            if (avcodec_open2(de_ctx, codec, NULL) < 0) {
                LOGD("video avcodec_open2() fail");
                releaseSources();
                return;
            }
            break;
        }
    }
    
    // 初始化编码器;因为最终是要写入到JPEG，所以使用的编码器ID为AV_CODEC_ID_MJPEG
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    en_ctx = avcodec_alloc_context3(codec);
    if (!en_ctx) {
        LOGD("avcodec_alloc_context3 fail");
        releaseSources();
        return;
    }
    // 设置编码参数
    AVStream *in_stream = in_fmt->streams[video_index];
    en_ctx->width = in_stream->codecpar->width;
    en_ctx->height = in_stream->codecpar->height;
    // 如果是编码后写入到图片中，那么比特率可以不用设置，不影响最终的结果(也不会影响图像清晰度)
    en_ctx->bit_rate = in_stream->codecpar->bit_rate;
    // 如果是编码后写入到图片中，那么帧率可以不用设置，不影响最终的结果
    en_ctx->framerate = in_stream->r_frame_rate;
    en_ctx->time_base = in_stream->time_base;
    // 对于MJPEG编码器来说，它支持的是YUVJ420P/YUVJ422P/YUVJ444P格式的像素
    en_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    
    // 初始化编码器上下文
    if (avcodec_open2(en_ctx, codec, NULL) < 0) {
        LOGD("avcodec_open2() fail");
        releaseSources();
        return;
    }
    
    
    // 创建用于输出JPG的封装器
    if (avformat_alloc_output_context2(&ou_fmt, NULL, NULL, dstPath.c_str()) < 0) {
        LOGD("avformat_alloc_output_context2");
        releaseSources();
        return;
    }
    
    /** 添加流
     *  对于图片封装器来说，也可以把它想象成只有一帧视频的视频封装器。所以它实际上也需要一路视频流，而事实上图片的流是视频流类型
     */
    AVStream *stream = avformat_new_stream(ou_fmt, NULL);
    // 设置流参数；直接从编码器拷贝参数即可
    if (avcodec_parameters_from_context(stream->codecpar, en_ctx) < 0) {
        LOGD("avcodec_parameters_from_context");
        releaseSources();
        return;
    }
    
    /** 初始化上下文
     *  对于写入JPG来说，它是不需要建立输出上下文IO缓冲区的的，所以avio_open2()没有调用到，但是最终一样可以调用av_write_frame()写入数据
     */
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstPath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2 fail");
            releaseSources();
            return;
        }
    }
    
    /** 为输出文件写入头信息
     *  不管是封装音视频文件还是图片文件，都需要调用此方法进行相关的初始化，否则av_write_frame()函数会崩溃
     */
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header() fail");
        releaseSources();
        return;
    }
    
    /** 创建视频像素转换上下文
     *  因为源视频的像素格式是yuv420p的，而jpg编码需要的像素格式是yuvj420p的，所以需要先进行像素格式转换
     */
    sws_ctx = sws_getContext(in_stream->codecpar->width, in_stream->codecpar->height, (enum AVPixelFormat)in_stream->codecpar->format,
                             en_ctx->width, en_ctx->height, en_ctx->pix_fmt,
                             0, NULL, NULL, NULL);
    if (!sws_ctx) {
        LOGD("sws_getContext fail");
        releaseSources();
        return;
    }
    
    // 创建编码解码用的AVFrame
    de_frame = av_frame_alloc();
    en_frame = av_frame_alloc();
    en_frame->width = en_ctx->width;
    en_frame->height = en_ctx->height;
    en_frame->format = en_ctx->pix_fmt;
    if (av_frame_get_buffer(en_frame, 0) < 0) {
        LOGD("av_frame_get_buffer fail");
        releaseSources();
        return;
    }
    if (av_frame_make_writable(en_frame) < 0) {
        LOGD("av_frame_make_writeable fail");
        releaseSources();
        return;
    }
    
    AVPacket *in_pkt = av_packet_alloc();
    AVPacket *ou_pkt = av_packet_alloc();
    AVRational time_base = in_fmt->streams[video_index]->time_base;
    AVRational frame_rate = in_fmt->streams[video_index]->r_frame_rate;
    
    // 一帧的时间戳
    int64_t delt = time_base.den/frame_rate.num;
    start_pts *= time_base.den;
    
    /** 因为想要截取的时间处的AVPacket并不一定是I帧，所以想要正确的解码，得先找到离想要截取的时间处往前的最近的I帧
     *  开始解码，直到拿到了想要获取的时间处的AVFrame
     *  AVSEEK_FLAG_BACKWARD 代表如果start_pts指定的时间戳处的AVPacket非I帧，那么就往前移动指针，直到找到I帧，那么
     *  当首次调用av_frame_read()函数时返回的AVPacket将为此I帧的AVPacket
     */
    if (av_seek_frame(in_fmt, video_index, start_pts, AVSEEK_FLAG_BACKWARD) < 0) {
        LOGD("av_seek_frame fail");
        releaseSources();
        return;
    }
    
    bool found = false;
    while (av_read_frame(in_fmt, in_pkt) == 0) {
        
        if (in_pkt->stream_index != video_index) {
            continue;
        }
        
        // 先解码
        avcodec_send_packet(de_ctx, in_pkt);
        LOGD("video pts %d(%s)",in_pkt->pts,av_ts2timestr(in_pkt->pts,&in_stream->time_base));
        while (true) {
            
            int ret = avcodec_receive_frame(de_ctx, de_frame);
            if (ret < 0) {
                break;
            }
            
            /** 解码得到的AVFrame中的pts和解码前的AVPacket中的pts是一一对应的，所以可以利用AVFrame中的pts来判断此帧是否在想要截取的时间范围内
             */
            LOGD("sucess pts %d",de_frame->pts);
            // 成功解码出来了
            // 取多帧视频并写入到文件
            static int i=0;
            delt = delt*num;
            // 去一帧帧视频并写入到文件
            if (abs(de_frame->pts - start_pts) < delt) {
                LOGD("找到了这一帧");
                
                // 因为源视频帧的格式和目标视频帧的格式可能不一致，所以这里需要转码
                ret = sws_scale(sws_ctx, de_frame->data, de_frame->linesize, 0, de_frame->height, en_frame->data, en_frame->linesize);
                if (ret < 0) {
                    LOGD("sws_scale fail");
                    releaseSources();
                    return;
                }
                
                
                if (getMore) {
                    // 重新编码
                    en_frame->pts = i;
                    avcodec_send_frame(en_ctx, en_frame);
                    // 拿到指定数目的AVPacket后再清空缓冲区
                    if (i > num) {
                        avcodec_send_frame(en_ctx, NULL);
                    }
                } else {
                    // 重新编码;因为只有一帧，所以这里直接写1 即可
                    en_frame->pts = 1;
                    avcodec_send_frame(en_ctx, en_frame);
                    // 因为只编码一帧，所以发送一帧视频后立马清空缓冲区
                    avcodec_send_frame(en_ctx, NULL);
                }
                
                ret = avcodec_receive_packet(en_ctx, ou_pkt);
                if (ret < 0) {
                    LOGD("fail ");
                    releaseSources();
                    return;
                }

                // 写入文件
                if(av_write_frame(ou_fmt, ou_pkt) < 0) {
                    LOGD("av_write_frame fail");
                    releaseSources();
                    return;
                }
                
                if (getMore) {
                    if (i>num) {
                        found = true;
                    }
                } else {
                    found = true;
                }
                
                break;
            }
        }
        
        av_packet_unref(in_pkt);
        if (found) {
            LOGD("has get jpg");
            break;
        }
    }
    
    /** 写入文件尾
     *  对于写入视频文件来说，此函数必须调用，但是对于写入JPG文件来说，不调用此函数也没关系；
     */
//    av_write_trailer(ou_fmt);
    
    // 释放资源
    releaseSources();
}
    
/** 多张图片合成为一段视频说明：
 *  1、ffmpeg对jpg的解封装和对视频的解封装一样，都封装到了avformat_xxxx系列接口里面。流程和解封装音视频的流程一模一样,对一张jpg图片的解封装可以理解为对只包含一帧
 *  视频的视频文件的解封装
 *  2、ffmpeg对jpeg的解封装支持模式匹配，例如对于name%3d.jpg进行解封装时(假如目录中包含的jpg列表为
 *  name001.jpg,name002.jpg,name004.jpg,........)，每次调用av_frame_read()函数，它将按照name001.jpg,name002.jpg,name004.jpg,........的顺序依次进行读取
 *  3、jpg对应的解封装器为ff_image2_demuxer，对应的编码器为ff_mjpeg_decoder
 */

/** 将前面视频生成的jpg合成mp4文件
 *  因为JPG的编码方式为AV_CODEC_ID_MJPEG，MP4如果采用h264编码，那么两者的编码方式是不一致的，所以就需要先解码再编码，具体流程为：
 *  1、先将JPG解码成AVFrame
 *  2、将JPG解码后的源像素格式YUVJ420P转换成x264编码需要的YUV420P像素格式
 *  3、再重新编码，然后再封装到mp4中
 */
void VideoJPG::doJpgToVideo(string srcPath,string dstPath)
{
    int video_index = -1;
    
    // 创建jpg的解封装上下文
    if (avformat_open_input(&in_fmt, srcPath.c_str(), NULL, NULL) < 0) {
        LOGD("avformat_open_input fail");
        return;
    }
    if (avformat_find_stream_info(in_fmt, NULL) < 0) {
        LOGD("avformat_find_stream_info()");
        releaseSources();
        return;
    }
    
    // 创建解码器及初始化解码器上下文用于对jpg进行解码
    for (int i=0; i<in_fmt->nb_streams; i++) {
        AVStream *stream = in_fmt->streams[i];
        /** 对于jpg图片来说，它里面就是一路视频流，所以媒体类型就是AVMEDIA_TYPE_VIDEO
         */
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (!codec) {
                LOGD("not find jpg codec");
                releaseSources();
                return;
            }
            de_ctx = avcodec_alloc_context3(codec);
            if (!de_ctx) {
                LOGD("jpg codec_ctx fail");
                releaseSources();
                return;
            }
            
            // 设置解码参数;文件解封装的AVStream中就包括了解码参数，这里直接流中拷贝即可
            if (avcodec_parameters_to_context(de_ctx, stream->codecpar) < 0) {
                LOGD("set jpg de_ctx parameters fail");
                releaseSources();
                return;
            }
            
            // 初始化解码器及解码器上下文
            if (avcodec_open2(de_ctx, codec, NULL) < 0) {
                LOGD("avcodec_open2() fail");
                releaseSources();
                return;
            }
            video_index = i;
            break;
        }
    }
    
    // 创建mp4文件封装器
    if (avformat_alloc_output_context2(&ou_fmt,NULL,NULL,dstPath.c_str()) < 0) {
        LOGD("MP2 muxer fail");
        releaseSources();
        return;
    }
    
    // 添加视频流
    AVStream *stream = avformat_new_stream(ou_fmt, NULL);
    video_ou_index = stream->index;
    
    // 创建h264的编码器及编码器上下文
    AVCodec *en_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!en_codec) {
        LOGD("encodec fail");
        releaseSources();
        return;
    }
    en_ctx = avcodec_alloc_context3(en_codec);
    if (!en_ctx) {
        LOGD("en_codec ctx fail");
        releaseSources();
        return;
    }
    // 设置编码参数
    AVStream *in_stream = in_fmt->streams[video_index];
    en_ctx->width = in_stream->codecpar->width;
    en_ctx->height = in_stream->codecpar->height;
    en_ctx->pix_fmt = (enum AVPixelFormat)in_stream->codecpar->format;
    en_ctx->bit_rate = 0.96*1000000;
    en_ctx->framerate = (AVRational){5,1};
    en_ctx->time_base = (AVRational){1,5};
    // 某些封装格式必须要设置，否则会造成封装后文件中信息的缺失
    if (ou_fmt->oformat->flags & AVFMT_GLOBALHEADER) {
        en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // x264编码特有
    if (en_codec->id == AV_CODEC_ID_H264) {
        // 代表了编码的速度级别
        av_opt_set(en_ctx->priv_data,"preset","slow",0);
        en_ctx->flags2 = AV_CODEC_FLAG2_LOCAL_HEADER;
    }
    
    // 初始化编码器及编码器上下文
    if (avcodec_open2(en_ctx,en_codec,NULL) < 0) {
        LOGD("encodec ctx fail");
        releaseSources();
        return;
    }
    
    // 设置视频流参数;对于封装来说，直接从编码器上下文拷贝即可
    if (avcodec_parameters_from_context(stream->codecpar, en_ctx) < 0) {
        LOGD("copy en_code parameters fail");
        releaseSources();
        return;
    }
    
    // 初始化封装器输出缓冲区
    if (!(ou_fmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open2(&ou_fmt->pb, dstPath.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
            LOGD("avio_open2 fail");
            releaseSources();
            return;
        }
    }
    
    // 创建像素格式转换器
    sws_ctx = sws_getContext(de_ctx->width, de_ctx->height, de_ctx->pix_fmt,
                             en_ctx->width, en_ctx->height, en_ctx->pix_fmt,
                             0, NULL, NULL, NULL);
    if (!sws_ctx) {
        LOGD("sws_getContext fail");
        releaseSources();
        return;
    }
    
    // 写入封装器头文件信息；此函数内部会对封装器参数做进一步初始化
    if (avformat_write_header(ou_fmt, NULL) < 0) {
        LOGD("avformat_write_header fail");
        releaseSources();
        return;
    }
    
    // 创建编解码用的AVFrame
    de_frame = av_frame_alloc();
    en_frame = av_frame_alloc();
    en_frame->width = en_ctx->width;
    en_frame->height = en_ctx->height;
    en_frame->format = en_ctx->pix_fmt;
    av_frame_get_buffer(en_frame, 0);
    av_frame_make_writable(en_frame);
    
    AVPacket *in_pkt = av_packet_alloc();
    while (av_read_frame(in_fmt, in_pkt) == 0) {
    
        
        if (in_pkt->stream_index != video_index) {
            continue;
        }
        
        // 先解码
        doDecode(in_pkt);
        av_packet_unref(in_pkt);
    }
    
    // 刷新解码缓冲区
    doDecode(NULL);
    av_write_trailer(ou_fmt);
    LOGD("结束。。。");
    
    // 释放资源
    releaseSources();
}
    
void VideoJPG::doDecode(AVPacket *in_pkt)
{
    static int num_pts = 0;
    // 先解码
    avcodec_send_packet(de_ctx, in_pkt);
    while (true) {
        int ret = avcodec_receive_frame(de_ctx, de_frame);
        if (ret == AVERROR_EOF) {
            doEncode(NULL);
            break;
        } else if(ret < 0) {
            break;
        }
        
        // 成功解码了；先进行格式转换然后再编码
        if(sws_scale(sws_ctx, de_frame->data, de_frame->linesize, 0, de_frame->height, en_frame->data, en_frame->linesize) < 0) {
            LOGD("sws_scale fail");
            releaseSources();
            return;
        }
        
        // 编码前要设置好pts的值，如果en_ctx->time_base为{1,fps}，那么这里pts的值即为帧的个数值
        en_frame->pts = num_pts++;
        doEncode(en_frame);
    }
    
}

void VideoJPG::doEncode(AVFrame *en_frame1)
{
    
    avcodec_send_frame(en_ctx, en_frame1);
    while (true) {
        AVPacket *ou_pkt = av_packet_alloc();
        if (avcodec_receive_packet(en_ctx, ou_pkt) < 0) {
            av_packet_unref(ou_pkt);
            break;
        }
        
        // 成功编码了;写入之前要进行时间基的转换
        AVStream *stream = ou_fmt->streams[video_ou_index];
        av_packet_rescale_ts(ou_pkt, en_ctx->time_base, stream->time_base);
        LOGD("video pts %d(%s)",ou_pkt->pts,av_ts2timestr(ou_pkt->pts, &stream->time_base));
        av_write_frame(ou_fmt, ou_pkt);
    }
}
