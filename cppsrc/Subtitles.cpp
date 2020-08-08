//
//  Subtitles.cpp
//  demo-mac
//
//  Created by apple on 2020/7/22.
//  Copyright © 2020 apple. All rights reserved.
//

#include "Subtitles.hpp"

Subtitles::Subtitles()
{
    vfmt = NULL;
    sfmt = NULL;
    ofmt = NULL;
    de_frame = NULL;
    en_video_ctx = NULL;
    de_video_ctx = NULL;
    src_filter_ctx = NULL;
    sink_filter_ctx = NULL;
    
    in_video_index = -1;in_audio_index = -1;
    ou_video_index = -1;ou_audio_index = -1;
}

Subtitles::~Subtitles()
{
    if (vfmt != NULL) {
        avformat_close_input(&vfmt);
    }
    if (sfmt) {
        avformat_close_input(&sfmt);
    }
    if (de_frame) {
        av_frame_free(&de_frame);
    }
    if (en_video_ctx) {
        avcodec_free_context(&en_video_ctx);
    }
    if (de_video_ctx) {
        avcodec_free_context(&de_video_ctx);
    }
    if (src_filter_ctx) {
        avfilter_free(src_filter_ctx);
        src_filter_ctx = NULL;
    }
    if (sink_filter_ctx) {
        avfilter_free(sink_filter_ctx);
        sink_filter_ctx = NULL;
    }
}

void Subtitles::releaseInternal()
{
    
}

/** 对于mkv的封装和解封装要开启ffmpeg的编译参数 --enable-muxer=matroska和--enable-demuxer=matroska
 *
 *  备注：不同格式的字幕ass/srt写入文件后，当用播放器打开的时候字幕的大小以及位置也有区别
 */
void Subtitles::addSubtitleStream(string videopath, string spath, string dstpath)
{
    if (dstpath.rfind(".mkv") != dstpath.length() - 4) {
        LOGD("can only suport .mkv file");
        return;
    }
    
    int ret = 0;
    // 打开视频流
    if (avformat_open_input(&vfmt,videopath.c_str(), NULL, NULL) < 0) {
        LOGD("avformat_open_input failed");
        return;
    }
    if (avformat_find_stream_info(vfmt, NULL) < 0) {
        LOGD("avformat_find_stream_info");
        releaseInternal();
        return;
    }
    
    if ((avformat_alloc_output_context2(&ofmt, NULL, NULL, dstpath.c_str())) < 0) {
        LOGD("avformat_alloc_output_context2() failed");
        releaseInternal();
        return;
    }
    
    int in_video_index = -1,in_audio_index = -1;
    int ou_video_index = -1,ou_audio_index = -1,ou_subtitle_index = -1;
    for (int i=0; i<vfmt->nb_streams; i++) {
        AVStream *stream = vfmt->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            in_video_index = i;
            AVStream *newstream = avformat_new_stream(ofmt, NULL);
            avcodec_parameters_copy(newstream->codecpar, stream->codecpar);
            newstream->codecpar->codec_tag = 0;
            ou_video_index = newstream->index;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVStream *newstream = avformat_new_stream(ofmt, NULL);
            avcodec_parameters_copy(newstream->codecpar, stream->codecpar);
            newstream->codecpar->codec_tag = 0;
            in_audio_index = i;
            ou_audio_index = newstream->index;
        }
    }
    if (!(ofmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open failed");
            releaseInternal();
            return;
        }
    }
    
    // 打开字幕流
    /** 遇到问题：调用avformat_open_input()时提示"avformat_open_input failed -1094995529(Invalid data found when processing input)"
     *  分析原因：编译ffmpeg库是没有将对应的字幕解析器添加进去比如(ff_ass_demuxer,ff_ass_muxer)
     *  解决方案：添加对应的编译参数
     */
    if ((ret = avformat_open_input(&sfmt,spath.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input failed %d(%s)",ret,av_err2str(ret));
        return;
    }
    if ((ret = avformat_find_stream_info(sfmt, NULL)) < 0) {
        LOGD("avformat_find_stream_info %d(%s)",ret,av_err2str(ret));
        releaseInternal();
        return;
    }
    
    if((ret = av_find_best_stream(sfmt, AVMEDIA_TYPE_SUBTITLE, -1, -1, NULL, 0))<0){
        LOGD("not find subtitle stream 0");
        releaseInternal();
        return;
    }
    AVStream *nstream = avformat_new_stream(ofmt, NULL);
    ret = avcodec_parameters_copy(nstream->codecpar, sfmt->streams[0]->codecpar);
    nstream->codecpar->codec_tag = 0;
    /** todo:zsz AV_DISPOSITION_xxx：ffmpeg.c中该选项可以控制字幕默认是否显示，不过这里貌似不可以，原因未知。
     */
//    nstream->disposition = sfmt->streams[0]->disposition;
    ou_subtitle_index = nstream->index;
    
    if(avformat_write_header(ofmt, NULL)<0){
        LOGD("avformat_write_header failed");
        releaseInternal();
        return;
    }
    av_dump_format(ofmt, 0, dstpath.c_str(), 1);
    
    /** 遇到问题：封装后生成的mkv文件字幕无法显示，封装时提示"[matroska @ 0x10381c000] Starting new cluster due to timestamp"
     *  分析原因：通过和ffmpeg.c中源码进行比对，后发现mvk对字幕写入的顺序有要求
     *  解决方案：将字幕写入放到音视频之前
     */
    AVPacket *inpkt2 = av_packet_alloc();
    while (av_read_frame(sfmt, inpkt2) >= 0) {
        
        AVStream *srcstream = sfmt->streams[0];
        AVStream *dststream = ofmt->streams[ou_subtitle_index];
        av_packet_rescale_ts(inpkt2, srcstream->time_base, dststream->time_base);
        inpkt2->stream_index = ou_subtitle_index;
        inpkt2->pos = -1;
        LOGD("pts %d",inpkt2->pts);
        if (av_write_frame(ofmt, inpkt2) < 0) {
            LOGD("subtitle av_write_frame failed");
            releaseInternal();
            return;
        }
        av_packet_unref(inpkt2);
    }
    
    AVPacket *inpkt = av_packet_alloc();
    while (av_read_frame(vfmt, inpkt) >= 0) {
        
        if (inpkt->stream_index == in_video_index) {
            AVStream *srcstream = vfmt->streams[in_video_index];
            AVStream *dststream = ofmt->streams[ou_video_index];
            av_packet_rescale_ts(inpkt, srcstream->time_base, dststream->time_base);
            inpkt->stream_index = ou_video_index;
            LOGD("inpkt %d",inpkt->pts);
            if (av_write_frame(ofmt, inpkt) < 0) {
                LOGD("video av_write_frame failed");
                releaseInternal();
                return;
            }
        } else if (inpkt->stream_index == in_audio_index) {
            AVStream *srcstream = vfmt->streams[in_audio_index];
            AVStream *dststream = ofmt->streams[ou_audio_index];
            av_packet_rescale_ts(inpkt, srcstream->time_base, dststream->time_base);
            inpkt->stream_index = ou_audio_index;
            if (av_write_frame(ofmt, inpkt) < 0) {
                LOGD("audio av_write_frame failed");
                releaseInternal();
                return;
            }
        }
        
        av_packet_unref(inpkt);
    }
    
    LOGD("over");
    av_write_trailer(ofmt);
    releaseInternal();
    
}

bool Subtitles::configConfpath(string confpath,string fontsPath,map<string,string>fontmaps)
{
    if (confpath.size() == 0) {
        LOGD("unvalide confpath");
        return false;
    }
    if (fopen(confpath.c_str(),"r")== NULL) {
        LOGD("confpath not exits!");
        return false;
    }
    
    FILE *file = NULL;
    if ((file = fopen(confpath.c_str(),"w+"))== NULL) {
        LOGD("can not create path %s",confpath.c_str());
        return false;
    }
    
    string fontnamemapping="";
    map<string, string>::iterator it;
    for (it = fontmaps.begin(); it != fontmaps.end(); it++) {
        string fontName = it->first;
        string mapedname= it->second;
        if (!fontName.empty() && !mapedname.empty()) {
            fontnamemapping += "\n";
            fontnamemapping += "    <match target=\"pattern\">\n";
            fontnamemapping += "        <test qual=\"any\" name=\"family\">\n";
            fontnamemapping += "            <string>"+fontName+"</string>\n";
            fontnamemapping += "        </test>\n";
            fontnamemapping += "        <edit name=\"family\" mode=\"assign\" binding=\"same\">\n";
            fontnamemapping += "            <string>"+mapedname+"</string>\n";
            fontnamemapping += "        </edit>\n";
            fontnamemapping += "    </match>\n";
        }
    }
    
    string conf = "";
    conf += "<?xml version=\"1.0\"?>\n";
    conf += "<!DOCTYPE fontconfig SYSTEM \"fonts.dtd\">\n";
    conf += "<fontconfig>\n";
    conf += "   <dir>.</dir>\n";
    conf += "   <dir>"+fontsPath+"</dir>\n";
    conf += fontnamemapping;
    conf += "</fontconfig>\n";
    
    if (fwrite(conf.c_str(), 1, conf.size(), file) <= 0) {
        LOGD("can not write to confpath,check permission");
        return false;
    }
    
    fclose(file);
    
    return true;
}
/** ffmpeg中字幕处理的滤镜有两个subtitles和drawtext。
 *  1、要想正确使用subtitles滤镜，编译ffmpeg时需要添加--enable-libass --enable-filter=subtitles配置参数，同时引入libass库。同时由于libass库又引用了freetype,fribidi外部库所以还需要同时编译这两个库，此外
 *  libass库根据操作系统的不同还引入不同的外部库，比如mac os系统则引入了CoreText.framework库,Linux则引入了fontconfig库，windows系统则引入了DirectWrite，或者添加--disable-require-system-font-provider
 *  代表不使用这些系统的库
 *  2、要想正确使用drawtext滤镜，编译ffmpeg时需要添加--enable-filter=drawtext同时要引入freetype和fribidi外部库
 *  3、所以libass和drawtext滤镜从本质上看都是调用freetype生成一张图片，然后再将图片和视频融合
 *
 *  与libass库字幕处理相关的三个库：
 *  1、text shaper相关：用来定义字体形状相关，fribidi和HarfBuzz两个库，其中fribidi速度较快，与字体库形状无关的一个库，libass默认，故HarfBuzz可以选择不编译
 *  2、字体库相关：CoreText(ios/mac)；fontconfig(linux/android/ios/mac);DirectWrite(windows)，用来创建字体。
 *  3、freetype：用于将字符串按照前面指定的字体以及字体形状渲染为字体图像(RGB格式，备注：它还可以将RGB格式最终输出为PNG，则需要编译libpng库)
 *
 *  所以libass则是对上面三个库的封装，用以更加方便的实现为一帧视频添加字幕图像的功能
 */
void Subtitles::addSubtitlesForVideo(string vpath, string spath, string dstpath,string confpath)
{
    int ret = 0;
    // 打开视频流
    if (avformat_open_input(&vfmt,vpath.c_str(), NULL, NULL) < 0) {
        LOGD("avformat_open_input failed");
        return;
    }
    if (avformat_find_stream_info(vfmt, NULL) < 0) {
        LOGD("avformat_find_stream_info");
        releaseInternal();
        return;
    }
    
    if((ret = avformat_alloc_output_context2(&ofmt, NULL, NULL, dstpath.c_str())) < 0) {
        LOGD("avformat_alloc_output_context2 failed");
        return;
    }
    
    for (int i=0; i<vfmt->nb_streams; i++) {
        AVStream *sstream = vfmt->streams[i];
        if (sstream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            in_video_index = i;
            // 添加新的视频流
            AVStream *nstream = avformat_new_stream(ofmt, NULL);
            ou_video_index = nstream->index;
            
            // 由于视频需要添加字幕，所以需要重新编解码，但是编码信息和源文件中一样
            AVCodec *codec = avcodec_find_decoder(sstream->codecpar->codec_id);
            if (!codec) {
                LOGD("not surport codec!");
                releaseInternal();
                return;
            }
            de_video_ctx = avcodec_alloc_context3(codec);
            if (!de_video_ctx) {
                LOGD("avcodec_alloc_context3 failed");
                releaseInternal();
                return;
            }
            // 设置解码参数，从源文件拷贝
            avcodec_parameters_to_context(de_video_ctx, sstream->codecpar);
            // 初始化解码器上下文
            if (avcodec_open2(de_video_ctx, codec, NULL) < 0) {
                LOGD("avcodec_open2 failed");
                releaseInternal();
                return;
            }
            
            // 创建编码器
            AVCodec *encodec = avcodec_find_encoder(sstream->codecpar->codec_id);
            if (!encodec) {
                LOGD("not surport encodec!");
                releaseInternal();
                return;
            }
            en_video_ctx = avcodec_alloc_context3(encodec);
            if (!en_video_ctx) {
                LOGD("avcodec_alloc_context3 failed");
                releaseInternal();
                return;
            }
            
            // 设置编码相关参数
            /** 遇到问题：生成视频前面1秒钟是灰色的
             *  分析原因：直接从源视频流拷贝编码参数到新的编码上下文中(即通过avcodec_parameters_to_context(en_video_ctx, sstream->codecpar);)而部分重要编码参数(如帧率，时间基)并不在codecpar
             *  中，所以导致参数缺失
             *  解决方案：额外设置时间基和帧率参数
             */
            avcodec_parameters_to_context(en_video_ctx, sstream->codecpar);
            // 设置帧率
            int fps = sstream->r_frame_rate.num;
            en_video_ctx->framerate = (AVRational){fps,1};
            // 设置时间基;
            en_video_ctx->time_base = sstream->time_base;
            // I帧间隔，决定了压缩率
            en_video_ctx->gop_size = 12;
            if (ofmt->oformat->flags & AVFMT_GLOBALHEADER) {
                en_video_ctx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            // 初始化编码器上下文
            if (avcodec_open2(en_video_ctx, encodec, NULL) < 0) {
                LOGD("avcodec_open2 failed");
                releaseInternal();
                return;
            }
            
            
            // 设置视频流相关参数
            avcodec_parameters_from_context(nstream->codecpar, en_video_ctx);
            nstream->codecpar->codec_tag = 0;
            
        } else if (sstream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            
            // 音频直接进行流拷贝
            in_audio_index = i;
            AVStream *nstream = avformat_new_stream(ofmt, NULL);
            avcodec_parameters_copy(nstream->codecpar, sstream->codecpar);
            ou_audio_index = nstream->index;
            nstream->codecpar->codec_tag = 0;
        }
    }
    
    if (in_video_index == -1) {
        LOGD("not has video stream");
        releaseInternal();
        return;
    }
    
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt->pb, dstpath.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open() failed");
            releaseInternal();
            return;
        }
    }
    
    av_dump_format(ofmt, -1, dstpath.c_str(), 1);
    
    // 写入头文件
    if (avformat_write_header(ofmt, NULL) < 0) {
        LOGD("avformat_write_header failed");
        releaseInternal();
        return;
    }
    
    // 初始化滤镜
    if (!initFilterGraph(spath,confpath)) {
        LOGD("");
        releaseInternal();
        return;
    }
    
    AVPacket *inpkt = av_packet_alloc();
    while (av_read_frame(vfmt, inpkt) >= 0) {
        
        if (inpkt->stream_index == in_video_index) {
            doDecodec(inpkt);
        } else if (inpkt->stream_index == in_audio_index) {
            // 进行时间基的转换
            av_packet_rescale_ts(inpkt, vfmt->streams[in_audio_index]->time_base, ofmt->streams[ou_audio_index]->time_base);
            inpkt->stream_index = ou_audio_index;
            LOGD("audio pts %d(%s)",inpkt->pts,av_ts2timestr(inpkt->pts,&ofmt->streams[ou_audio_index]->time_base));
            av_write_frame(ofmt, inpkt);
        }
        
        av_packet_unref(inpkt);
    }
    
    LOGD("finish !");
    doDecodec(NULL);
    av_write_trailer(ofmt);
    releaseInternal();
    
}

/** 要使用subtitles和drawtext滤镜到ffmpeg中，则编译ffmpeg库时需要开启如下选项：
 *  1、字幕编解码器 --enable-encoder=ass --enable-decoder=ass --enable-encoder=srt --enable-decoder=srt --enable-encoder=webvtt --enable-decoder=webvtt；
 *  2、字幕解封装器 --enable-muxer=ass --enable-demuxer=ass --enable-muxer=srt --enable-demuxer=srt --enable-muxer=webvtt --enable-demuxer=webvtt
 *  3、滤镜选项  --enable-filter=drawtext --enable-libfreetype --enable-libass --enable-filter=subtitles
 *
 *  备注：以上字幕编解码器以及字幕解封装器可以只使用一个即可，代表只能使用一个字幕格式。具体参考编译脚本
 */
bool Subtitles::initFilterGraph(string spath,string confpath)
{
    graph = avfilter_graph_alloc();
    int ret = 0;
    AVStream *stream = vfmt->streams[in_video_index];
    // 输入滤镜
    const AVFilter *src_filter = avfilter_get_by_name("buffer");
    char desc[400];
    sprintf(desc,"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",stream->codecpar->width,stream->codecpar->height,stream->codecpar->format,stream->time_base.num,stream->time_base.den);
    ret = avfilter_graph_create_filter(&src_filter_ctx, src_filter, "buffer0", desc, NULL, graph);
    if (ret < 0) {
        LOGD("init src filter failed");
        return false;
    }

    // 输出滤镜
    const AVFilter *sink_filter = avfilter_get_by_name("buffersink");
    ret = avfilter_graph_create_filter(&sink_filter_ctx, sink_filter, "buffersink0", NULL, NULL, graph);
    if (ret < 0) {
        LOGD("buffersink init failed");
        return false;
    }
    
    /** 遇到问题：当使用libass库来合成字幕时无法生成字幕
     *  分析原因：libass使用fontconfig库来匹配字体，而程序中没有指定字体匹配用的描述文件
     *  解决方案：设置FONTCONFIG_FILE的值
     *
     *  fontconfig工作原理：fontconfig通过环境变量FONTCONFIG_FILE来找到指定的fonts.conf文件(该文件的指定了字体文件(ttf,ttc等)的目录，以及字体fallback的规则)，最终选择指定的字体文件
     *  font fallback:如果某个字符在指定的字体库中不存在，那么就需要找到能够显示此字符的备用字体库，fontconfig就是专门做此事的。
     *
     *  备注：
     *  1、mac下 系统字体库的路径为：/System/Library/Fonts
     *  2、iOS下 系统字体库的路径为：ios系统字体不允许访问
     *  3、安卓下 系统字体库的路为：/system/fonts
     *  4、Ubuntu下 系统字体库的路径为：/usr/share/fonts
     *  不同系统支持的字体库可能不一样，由于fontconfig的字体fallback机制，如果不自定义自己的字体库，可能不同系统最终因为选择的字体库不一样导致合成字幕也不一样。
     *  所以解决办法就是统一用于各个平台的字体库，然后自定义fontconfig的字体库的搜索路径
     */
    // 滤镜描述符
    setenv("FONTCONFIG_FILE",confpath.c_str(), 0);
    char filter_des[400];
    sprintf(filter_des, "subtitles=filename=%s",spath.c_str());
    AVFilterInOut *inputs = avfilter_inout_alloc();
    AVFilterInOut *ouputs = avfilter_inout_alloc();
    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink_filter_ctx;
    inputs->next = NULL;
    inputs->pad_idx = 0;
    
    ouputs->name = av_strdup("in");
    ouputs->filter_ctx = src_filter_ctx;
    ouputs->next = NULL;
    ouputs->pad_idx = 0;
    
    if (avfilter_graph_parse_ptr(graph, filter_des, &inputs, &ouputs, NULL) < 0) {
        LOGD("avfilter_graph_parse_ptr failed");
        return false;
    }
    
    av_buffersink_set_frame_size(sink_filter_ctx, en_video_ctx->frame_size);
    
    // 初始化滤镜
    if (avfilter_graph_config(graph, NULL) < 0) {
        LOGD("avfilter_graph_config failed");
        return false;
    }
    
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&ouputs);
    
    return true;
}

void Subtitles::doDecodec(AVPacket *pkt)
{
    if (!de_frame) {
        de_frame = av_frame_alloc();
    }
    int ret = avcodec_send_packet(de_video_ctx, pkt);
    while (true) {
        ret = avcodec_receive_frame(de_video_ctx, de_frame);
        if (ret == AVERROR_EOF) {
            // 说明已经没有数据了;清空
            //解码成功送入滤镜进行处理
            if((ret = av_buffersrc_add_frame_flags(src_filter_ctx, NULL, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
                LOGD("av_buffersrc_add_frame_flags failed");
                break;
            }
            break;
        } else if (ret < 0) {
            break;
        }
        
        //解码成功送入滤镜进行处理
        if((ret = av_buffersrc_add_frame_flags(src_filter_ctx, de_frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0) {
            LOGD("av_buffersrc_add_frame_flags failed");
            break;
        }

        while (true) {
            AVFrame *enframe = av_frame_alloc();
            ret = av_buffersink_get_frame(sink_filter_ctx, enframe);
            if (ret == AVERROR_EOF) {
                // 说明结束了
                LOGD("avfilter endeof");
                // 清空编码器
                doEncodec(NULL);
                // 释放内存
                av_frame_unref(enframe);
            } else if (ret < 0) {
                // 释放内存
                av_frame_unref(enframe);
                break;
            }

            // 进行重新编码
            doEncodec(enframe);
            // 释放内存
            av_frame_unref(enframe);
        }
    }
}

void Subtitles::doEncodec(AVFrame *frame)
{
    int ret = avcodec_send_frame(en_video_ctx, frame);
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        ret = avcodec_receive_packet(en_video_ctx, pkt);
        if (ret < 0) {
            av_packet_unref(pkt);
            break;
        }
        
        // 写入数据
        av_packet_rescale_ts(pkt, en_video_ctx->time_base, ofmt->streams[ou_video_index]->time_base);
        pkt->stream_index = ou_video_index;
        LOGD("video pts %d(%s)",pkt->pts,av_ts2timestr(pkt->pts,&ofmt->streams[ou_video_index]->time_base));
        av_write_frame(ofmt, pkt);
        
        av_packet_unref(pkt);
    }
}
