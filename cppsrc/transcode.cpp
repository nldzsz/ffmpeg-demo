//
//  transcode.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/3.
//  Copyright © 2020 apple. All rights reserved.
//

#include "transcode.hpp"

Transcode::Transcode():dstPath(""),srcPath(""),inFmtCtx(NULL),ouFmtCtx(NULL),
video_in_stream_index(-1),video_ou_stream_index(-1),audio_in_stream_index(-1),audio_ou_stream_index(-1),video_pts(0),audio_pts(0),
src_video_id(AV_CODEC_ID_NONE),src_audio_id(AV_CODEC_ID_NONE),
video_de_ctx(NULL),audio_de_ctx(NULL),video_en_ctx(NULL),audio_en_ctx(NULL),swr_ctx(NULL),sws_ctx(NULL),
video_de_frame(NULL),audio_de_frame(NULL),video_en_frame(NULL),audio_en_frame(NULL),
last_video_pts(0),last_audio_pts(0),audio_need_convert(false),video_need_convert(false)
{
    
}

Transcode::~Transcode()
{
    
}

/** 实现MP4转换成MOV,FLV,TS,AVI文件，不改变编码方式
 */
void Transcode::doExtensionTranscode(string srcPath,string dstPath)
{
    AVFormatContext *in_fmtCtx = NULL,*ou_fmtCtx = NULL;
    int video_in_stream_index = -1,audio_in_stream_index = -1;
    int video_ou_stream_index = -1,audio_ou_stream_index = -1;
    
    int ret = 0;
    if ((ret = avformat_open_input(&in_fmtCtx,srcPath.c_str(),NULL,NULL)) < 0) {
        LOGD("avformat_open_input fail %s",av_err2str(ret));
        return;
    }
    if (avformat_find_stream_info(in_fmtCtx,NULL) < 0) {
        LOGD("avformat_find_stream_info fail %s",av_err2str(ret));
        return;
    }
    
    av_dump_format(in_fmtCtx, 0, srcPath.c_str(), 0);
    
    
    if ((ret = avformat_alloc_output_context2(&ou_fmtCtx,NULL,NULL,dstPath.c_str())) < 0) {
        LOGD("avformat_alloc_output_context2 fail %s",av_err2str(ret));
        return;
    }
    
    for (int i=0; i<in_fmtCtx->nb_streams; i++) {
        AVStream *stream = in_fmtCtx->streams[i];
        
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_in_stream_index == -1) {
            video_in_stream_index = i;
            
            // 添加流信息
            AVStream *newStream = avformat_new_stream(ou_fmtCtx,NULL);
            video_ou_stream_index = newStream->index;
            // 将编码信息拷贝过来
            LOGD("fourcc %s",av_fourcc2str(newStream->codecpar->codec_tag));
            if ((ret = avcodec_parameters_copy(newStream->codecpar, stream->codecpar)) <0) {
                LOGD("avcodec_parameters_copy fail %s",av_err2str(ret));
                return;
            }
            /** 遇到问题：mp4转换为avi时，播放提示“reference count 1 overflow,chroma_log2_weight_denom 1696 is out of range”
             *  分析原因：之前的写法是没有加下面的判断条件直接为newStream->codecpar->codec_tag = 0;这样avi将选择默认的h264作为码流格式(源mp4中为avc1)，导致两边不一样
             *  解决方案：加如下判断
            */
            uint32_t src_codec_tag = stream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmtCtx->oformat->codec_tag, src_codec_tag) != newStream->codecpar->codec_id) {
                newStream->codecpar->codec_tag = 0;
            }
        }
        
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_in_stream_index == -1) {
            audio_in_stream_index = i;
            AVStream *newStream = avformat_new_stream(ou_fmtCtx,NULL);
            audio_ou_stream_index = newStream->index;
            if ((ret = avcodec_parameters_copy(newStream->codecpar, stream->codecpar)) <0) {
                LOGD("avcodec_parameters_copy2 fail %s",av_err2str(ret));
                return;
            }
            /** 遇到问题：avformat_write_header()函数提示"Tag mp4a incompatible with output codec id '86018'"
             *  分析原因：code_tag代表了音视频数据采用的码流格式。拿aac举例，AVI和MP4都支持aac编码的音频数据存储，MP4支持MKTAG('m', 'p', '4',
             *  'a')码流格式的aac流，avi支持(0x00ff,0x1600,0x706d等)，显然两者是不一样的，上面avcodec_parameters_copy()就相当于让封装和解封装的
             *  code_tag标签一模一样，所以造成了不一致的问题
             *  解决方案：将codecpar->codec_tag=0，系统会默认选择第一个匹配编码方式的codec_tag值
             */
            uint32_t src_codec_tag = stream->codecpar->codec_tag;
            if (av_codec_get_id(ou_fmtCtx->oformat->codec_tag, src_codec_tag) != newStream->codecpar->codec_id) {
                newStream->codecpar->codec_tag = 0;
            }
        }
    }
    
    // 当flags没有AVFMT_NOFILE标记的时候才能调用avio_open2()函数进行初始化
    if (!(ou_fmtCtx->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open2(&ou_fmtCtx->pb,dstPath.c_str(),AVIO_FLAG_WRITE,NULL,NULL)) < 0) {
            LOGD("avio_open2 fail %d",ret);
            return;
        }
    }
    
    // 写入文件头信息
    if ((ret = avformat_write_header(ou_fmtCtx, NULL)) < 0) {
        LOGD("avformat_write_header fail %s",av_err2str(ret));
        return;
    }
    av_dump_format(ou_fmtCtx, 0, dstPath.c_str(), 1);
    
    AVPacket *in_packet = av_packet_alloc();
    while ((av_read_frame(in_fmtCtx, in_packet)) == 0) {
        if (in_packet->stream_index != video_in_stream_index && in_packet->stream_index != audio_in_stream_index) {
            continue;
        }
        LOGD("write packet index %d size %d",in_packet->stream_index,in_packet->size);
        AVStream *in_stream = in_fmtCtx->streams[in_packet->stream_index];
        AVStream *ou_stream = NULL;
        if (in_stream->index == video_in_stream_index) {
            ou_stream = ou_fmtCtx->streams[video_ou_stream_index];
        } else {
            ou_stream = ou_fmtCtx->streams[audio_ou_stream_index];
        }
        
        /** 每个packet中pts,dts,duration 转换成浮点数时间的公式(以pts为例)：pts * timebase.num/timebase.den
         */
        in_packet->pts = av_rescale_q_rnd(in_packet->pts,in_stream->time_base,ou_stream->time_base,AV_ROUND_UP);
        in_packet->dts = av_rescale_q_rnd(in_packet->dts,in_stream->time_base,ou_stream->time_base,AV_ROUND_UP);
        in_packet->duration = av_rescale_q_rnd(in_packet->duration, in_stream->time_base, ou_stream->time_base, AV_ROUND_UP);
        in_packet->stream_index = ou_stream->index;
        /** 遇到问题：提示"H.264 bitstream malformed, no startcode found, use the video bitstream filter 'h264_mp4toannexb' to fix it ('-bsf:v
         *  h264_mp4toannexb' option with ffmpeg)"
         *  分析原因：MP4中h264编码的视频码流格式为avcc(即每个NALU的前面都加了四个大端序的字节，表示每个NALU的长度)，而avi中h264
         *  编码的视频码流格式为annexb(即每个NALU的前面是0001或者001开头的开始码)，两者不一致。
         *  解决方案：将开头的四个字节替换掉
         */
//        if (in_packet->stream_index == video_ou_stream_index) {
//            uint8_t *orgData = in_packet->data;
//            orgData[0] = 0;
//            orgData[1] = 0;
//            orgData[2] = 0;
//            orgData[3] = 1;
//        }
//        printBuffertoHex(in_packet->data,in_packet->size);
        av_write_frame(ou_fmtCtx, in_packet);
        
        av_packet_unref(in_packet);
    }
    LOGD("over finish");
    av_write_trailer(ou_fmtCtx);
    avformat_close_input(&in_fmtCtx);
    avformat_free_context(ou_fmtCtx);
}

/** 做转码(包括编码方式，码率，分辨率，采样率，采样格式等等的转换)
 */
void Transcode::doTranscode(string sPath,string dPath)
{
    int ret = 0;
    srcPath = sPath;
    dstPath = dPath;
    // 做编码方式的转码，目标封装格式要支持 第一个参数代表是否引用源文件的参数，如果为true则忽略第二个参数，否则使用第二个参数
    // 视频
    dst_video_id = Par_CodeId(false,AV_CODEC_ID_MPEG4);
    dst_width =  Par_Int32t(false,1280);
    dst_height = Par_Int32t(false,720);
    dst_video_bit_rate = Par_Int64t(true,9000000);       // 0.9Mb/s
    dst_pix_fmt = Par_PixFmt(false,AV_PIX_FMT_NV12);
    dst_fps = Par_Int32t(true,50);  //fps
    video_need_transcode = true;
    
    // 音频
    dst_audio_id =  Par_CodeId(false,AV_CODEC_ID_AAC);
    dst_audio_bit_rate = Par_Int64t(true,128*1000);
    dst_sample_rate = Par_Int32t(false,48000);    // 44.1khz
    dst_sample_fmt = Par_SmpFmt(false,AV_SAMPLE_FMT_FLTP);
    dst_channel_layout = Par_Int64t(false,AV_CH_LAYOUT_STEREO);   // 双声道
    audio_need_transcode = true;
    
    // 打开输入文件及输出文件的上下文
    if (!openFile()) {
        LOGD("openFile()");
        return;
    }
    
    // 为输出文件添加视频流
    if (video_in_stream_index != -1 && video_need_transcode) {
        if (!add_video_stream()) {
            LOGD("add_video_stream fai");
            return;
        }
    }
    
    // 为输出文件添加音频流
    if (audio_in_stream_index != -1 && audio_need_transcode) {
        if (!add_audio_stream()) {
            LOGD("add_audio_stream()");
            return;
        }
    }
    
    av_dump_format(ouFmtCtx, 0, dstPath.c_str(), 1);
    
    // 打开解封装的上下文
    if (!(ouFmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ouFmtCtx->pb, dstPath.c_str(), AVIO_FLAG_WRITE) < 0) {
            LOGD("avio_open fail");
            releaseSources();
            return;
        }
    }
    
    
    // 写入头信息
    /** 遇到问题：avformat_write_header()崩溃
     *  分析原因：没有调用avio_open()函数
     *  解决方案：写成 !(ouFmtCtx->flags & AVFMT_NOFILE)即可
     */
    if ((ret = avformat_write_header(ouFmtCtx, NULL)) < 0) {
        LOGD("avformat_write_header fail %d",ret);
        releaseSources();
        return;
    }
    
    // 读取源文件中的音视频数据进行解码
    AVPacket *inPacket = av_packet_alloc();
    while (av_read_frame(inFmtCtx, inPacket) == 0) {
        // 说明读取的视频数据
        if (inPacket->stream_index == video_in_stream_index && video_need_transcode) {
            doDecodeVideo(inPacket);
        }

        // 说明读取的音频数据
        if (inPacket->stream_index == audio_in_stream_index && audio_need_transcode) {
            doDecodeAudio(inPacket);
        }
        
        // 因为每一次读取的AVpacket的数据大小不一样，所以用完之后要释放
        av_packet_unref(inPacket);
    }
    LOGD("文件读取完毕");
    
    // 刷新解码缓冲区，获取缓冲区中数据
    if (video_in_stream_index != -1 && video_need_transcode) {
        doDecodeVideo(NULL);
    }
    if (audio_in_stream_index != -1 && audio_need_transcode) {
        doDecodeAudio(NULL);
    }
    
    // 写入尾部信息
    if (ouFmtCtx) {
        av_write_trailer(ouFmtCtx);
    }
    LOGD("结束");
    
    // 释放资源
    releaseSources();
}

bool Transcode::openFile()
{
    if (!srcPath.length()) {
        return false;
    }
    int ret = 0;
    // 打开输入文件
    if ((ret = avformat_open_input(&inFmtCtx, srcPath.c_str(), NULL, NULL)) < 0) {
        LOGD("avformat_open_input() fail");
        releaseSources();
        return false;
    }
    if ((ret = avformat_find_stream_info(inFmtCtx, NULL)) < 0) {
        LOGD("avformat_find_stream_info fail %d",ret);
        releaseSources();
        return false;
    }
    // 输出输入文件信息
    av_dump_format(inFmtCtx,0,srcPath.c_str(),0);
    
    for (int i=0; i<inFmtCtx->nb_streams; i++) {
        AVCodecParameters *codecpar = inFmtCtx->streams[i]->codecpar;
        if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_in_stream_index == -1) {
            src_video_id = codecpar->codec_id;
            video_in_stream_index = i;
        }
        if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_in_stream_index == -1) {
            src_audio_id = codecpar->codec_id;
            audio_in_stream_index = i;
        }
    }
    
    // 打开输出流
    if ((ret = avformat_alloc_output_context2(&ouFmtCtx, NULL, NULL, dstPath.c_str())) < 0) {
        LOGD("avformat_alloc_context fail");
        releaseSources();
        return false;
    }
    // 检查目标编码方式是否被支持
    if (video_in_stream_index != -1 && av_codec_get_tag(ouFmtCtx->oformat->codec_tag, dst_video_id.par_val.codecId) == 0) {
        LOGD("video tag not found");
        releaseSources();
        return false;
    }
    if (audio_in_stream_index != -1 && av_codec_get_tag(ouFmtCtx->oformat->codec_tag,dst_audio_id.par_val.codecId) == 0) {
        LOGD("audio tag not found");
        releaseSources();
        return false;
    }
    
    return true;
}

static int select_sample_rate(AVCodec *codec,int rate)
{
    int best_rate = 0;
    int deft_rate = 44100;
    bool surport = false;
    const int* p = codec->supported_samplerates;
    if (!p) {
        return deft_rate;
    }
    while (*p) {
        best_rate = *p;
        if (*p == rate) {
            surport = true;
            break;
        }
        p++;
    }
    
    if (best_rate != rate && best_rate != 0 && best_rate != deft_rate) {
        return deft_rate;
    }
    
    return best_rate;
}

static enum AVSampleFormat select_sample_format(AVCodec *codec,enum AVSampleFormat fmt)
{
    enum AVSampleFormat retfmt = AV_SAMPLE_FMT_NONE;
    enum AVSampleFormat deffmt = AV_SAMPLE_FMT_FLTP;
    const enum AVSampleFormat * fmts = codec->sample_fmts;
    if (!fmts) {
        return deffmt;
    }
    while (*fmts != AV_SAMPLE_FMT_NONE) {
        retfmt = *fmts;
        if (retfmt == fmt) {
            break;
        }
        fmts++;
    }
    
    if (retfmt != fmt && retfmt != AV_SAMPLE_FMT_NONE && retfmt != deffmt) {
        return deffmt;
    }
    
    return retfmt;
}

static int64_t select_channel_layout(AVCodec *codec,int64_t ch_layout)
{
    int64_t retch = 0;
    int64_t defch = AV_CH_LAYOUT_STEREO;
    const uint64_t * chs = codec->channel_layouts;
    if (!chs) {
        return defch;
    }
    while (*chs) {
        retch = *chs;
        if (retch == ch_layout) {
            break;
        }
        chs++;
    }
    
    if (retch != ch_layout && retch != AV_SAMPLE_FMT_NONE && retch != defch) {
        return defch;
    }
    
    return retch;
}

static enum AVPixelFormat select_pixel_format(AVCodec *codec,enum AVPixelFormat fmt) {
    enum AVPixelFormat retpixfmt = AV_PIX_FMT_NONE;
    enum AVPixelFormat defaltfmt = AV_PIX_FMT_YUV420P;
    const enum AVPixelFormat *fmts = codec->pix_fmts;
    if (!fmts) {
        return defaltfmt;
    }
    while (*fmts != AV_PIX_FMT_NONE) {
        retpixfmt = *fmts;
        if (retpixfmt == fmt) {
            break;
        }
        fmts++;
    }
    
    if (retpixfmt != fmt && retpixfmt != AV_PIX_FMT_NONE && retpixfmt != defaltfmt) {
        return defaltfmt;
    }
    
    return retpixfmt;
}

bool Transcode::add_video_stream()
{
    AVCodec *codec = avcodec_find_encoder(dst_video_id.par_val.codecId);
    if (codec == NULL) {
        LOGD("video code find fail");
        releaseSources();
        return false;
    }
    
    AVStream *stream = avformat_new_stream(ouFmtCtx, NULL);
    if (!stream) {
        LOGD("avformat_new_stream fail");
        return false;
    }
    video_ou_stream_index = stream->index;
    
    video_en_ctx = avcodec_alloc_context3(codec);
    if (video_en_ctx == NULL) {
        releaseSources();
        LOGD("video codec not found");
        return false;
    }
    
    AVCodecParameters *inpar = inFmtCtx->streams[video_in_stream_index]->codecpar;
    // 设置编码相关参数
    video_en_ctx->codec_id = codec->id;
    // 设置码率
    video_en_ctx->bit_rate = dst_video_bit_rate.copy?inpar->bit_rate:dst_video_bit_rate.par_val.i64_val;
    // 设置视频宽
    video_en_ctx->width = dst_width.copy?inpar->width:dst_width.par_val.i32_val;
    // 设置视频高
    video_en_ctx->height = dst_height.copy?inpar->height:dst_height.par_val.i32_val;
    // 设置帧率
    int fps = dst_fps.copy?inFmtCtx->streams[video_in_stream_index]->r_frame_rate.num:dst_fps.par_val.i32_val;
    video_en_ctx->framerate = (AVRational){fps,1};
    // 设置时间基;
    stream->time_base = (AVRational){1,video_en_ctx->framerate.num};
    video_en_ctx->time_base = stream->time_base;
    // I帧间隔，决定了压缩率
    video_en_ctx->gop_size = 12;
    // 设置视频像素格式
    enum AVPixelFormat want_pix_fmt = dst_pix_fmt.copy?(AVPixelFormat)inpar->format:dst_pix_fmt.par_val.pix_fmt;
    enum AVPixelFormat result_pix_fmt = select_pixel_format(codec, want_pix_fmt);
    if (result_pix_fmt == AV_PIX_FMT_NONE) {
        LOGD("can not surport pix fmt");
        releaseSources();
        return false;
    }
    video_en_ctx->pix_fmt = result_pix_fmt;
    // 每组I帧间B帧的最大个数
    if (codec->id == AV_CODEC_ID_MPEG2VIDEO) {
        video_en_ctx->max_b_frames = 2;
    }
    /* Needed to avoid using macroblocks in which some coeffs overflow.
    * This does not happen with normal video, it just happens here as
    * the motion of the chroma plane does not match the luma plane. */
    if (codec->id == AV_CODEC_ID_MPEG1VIDEO) {
        video_en_ctx->mb_decision = 2;
    }
    
    // 判断是否需要进行格式转换
    if (result_pix_fmt != (enum AVPixelFormat)inpar->format || video_en_ctx->width != inpar->width  || video_en_ctx->height != inpar->height) {
        video_need_convert = true;
    }
    
    // 对应一些封装器需要添加这个标记
    /** 遇到问题：生成的mp4或者mov文件没有显示用于预览的小图片
     *  分析原因：之前代编码器器flags标记设置的为AVFMT_GLOBALHEADER(错了)，正确的值应该是AV_CODEC_FLAG_GLOBAL_HEADER
     *  解决思路：使用如下代码设置AV_CODEC_FLAG_GLOBAL_HEADER
     */
    if (ouFmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
//        video_en_ctx->flags |= AVFMT_GLOBALHEADER;
        video_en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // x264编码特有参数
    if (video_en_ctx->codec_id == AV_CODEC_ID_H264) {
        av_opt_set(video_en_ctx->priv_data,"preset","slow",0);
        video_en_ctx->flags |= AV_CODEC_FLAG2_LOCAL_HEADER;
    }
    
    int ret = 0;
    if ((ret = avcodec_open2(video_en_ctx, video_en_ctx->codec, NULL)) < 0) {
        LOGD("video encode open2() fail %d",ret);
        releaseSources();
        return false;
    }
    
    if((ret = avcodec_parameters_from_context(stream->codecpar, video_en_ctx)) < 0) {
        releaseSources();
        LOGD("video avcodec_parameters_from_context fail");
        return false;
    }
    
    return true;
}

bool Transcode::add_audio_stream()
{
    if (ouFmtCtx == NULL) {
        LOGD("audio outformat NULL");
        releaseSources();
        return false;
    }
    
    // 添加一个音频流
    AVStream *stream = avformat_new_stream(ouFmtCtx, NULL);
    if (!stream) {
        LOGD("avformat_new_stream fail");
        return false;
    }
    audio_ou_stream_index = stream->index;
    
    AVCodecParameters *incodecpar = inFmtCtx->streams[audio_in_stream_index]->codecpar;
    AVCodec *codec = avcodec_find_encoder(dst_audio_id.par_val.codecId);
    AVCodecContext *ctx = avcodec_alloc_context3(codec);
    if (ctx == NULL) {
        LOGD("audio codec ctx NULL");
        releaseSources();
        return false;
    }
    // 设置音频编码参数
    // 采样率
    int want_sample_rate = dst_sample_rate.copy?incodecpar->sample_rate:dst_sample_rate.par_val.i32_val;
    int relt_sample_rate = select_sample_rate(codec, want_sample_rate);
    if (relt_sample_rate == 0) {
        LOGD("cannot surpot sample_rate");
        releaseSources();
        return false;
    }
    ctx->sample_rate = relt_sample_rate;
    // 采样格式
    enum AVSampleFormat want_sample_fmt = dst_sample_fmt.copy?(enum AVSampleFormat)incodecpar->format:dst_sample_fmt.par_val.smp_fmt;
    enum AVSampleFormat relt_sample_fmt = select_sample_format(codec, want_sample_fmt);
    if (relt_sample_fmt == AV_SAMPLE_FMT_NONE) {
        LOGD("cannot surpot sample_fmt");
        releaseSources();
        return false;
    }
    ctx->sample_fmt  = relt_sample_fmt;
    // 声道格式
    int64_t want_ch = dst_channel_layout.copy?incodecpar->channel_layout:dst_channel_layout.par_val.i64_val;
    int64_t relt_ch = select_channel_layout(codec, want_ch);
    if (!relt_ch) {
        LOGD("cannot surpot channel_layout");
        releaseSources();
        return false;
    }
    ctx->channel_layout = relt_ch;
    // 声道数
    ctx->channels = av_get_channel_layout_nb_channels(ctx->channel_layout);
    // 编码后的码率
    ctx->bit_rate = dst_audio_bit_rate.copy?incodecpar->bit_rate:dst_audio_bit_rate.par_val.i32_val;
    // frame_size 这里不用设置，调用avcodec_open2()函数后会根据编码类型自动设置
//    ctx->frame_size = incodecpar->frame_size;
    // 设置时间基
    ctx->time_base = (AVRational){1,ctx->sample_rate};
    stream->time_base = ctx->time_base;
    
    // 保存
    audio_en_ctx = ctx;
    
    // 设置编码的相关标记，这样进行封装的时候不会漏掉一些元信息
    if (ouFmtCtx->oformat->flags & AVFMT_GLOBALHEADER) {
        audio_en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // 初始化编码器
    if (avcodec_open2(ctx, codec, NULL) < 0) {
        LOGD("audio avcodec_open2() fail");
        releaseSources();
        return false;
    }
    
    if (audio_en_ctx->frame_size != incodecpar->frame_size || relt_sample_fmt != incodecpar->format || relt_ch != incodecpar->channel_layout || relt_sample_rate != incodecpar->sample_rate) {
        audio_need_convert = true;
    }
    
    int ret = 0;
    // 将编码信息设置到音频流中
    if ((ret = avcodec_parameters_from_context(stream->codecpar, ctx)) < 0) {
        LOGD("audio copy audio stream fail");
        releaseSources();
        return false;
    }
    
    // 初始化重采样
    if (audio_need_convert) {
        swr_ctx = swr_alloc_set_opts(NULL, ctx->channel_layout, ctx->sample_fmt, ctx->sample_rate, incodecpar->channel_layout, (enum AVSampleFormat)incodecpar->format, incodecpar->sample_rate, 0, NULL);
        if ((ret = swr_init(swr_ctx)) < 0) {
            LOGD("swr_alloc_set_opts() fail %d",ret);
            releaseSources();
            return false;
        }
    }
    
    return true;
}

void Transcode::doDecodeVideo(AVPacket *inpacket)
{
    if (src_video_id == AV_CODEC_ID_NONE) {
        LOGD("src has not video");
        return;
    }
    
    int ret = 0;
    if (video_de_ctx == NULL) {
        AVCodec *codec = avcodec_find_decoder(src_video_id);
        video_de_ctx = avcodec_alloc_context3(codec);
        if (video_de_ctx == NULL) {
            LOGD("video decodec context create fail");
            releaseSources();
            return;
        }
        
        // 设置解码参数;这里是直接从AVCodecParameters中拷贝
        AVCodecParameters *codecpar = inFmtCtx->streams[video_in_stream_index]->codecpar;
        if ((ret = avcodec_parameters_to_context(video_de_ctx, codecpar)) < 0) {
           LOGD("avcodec_parameters_to_context fail %d",ret);
           releaseSources();
           return;
        }
        // 初始化解码器
        if (avcodec_open2(video_de_ctx, codec, NULL) < 0) {
           releaseSources();
           LOGD("video avcodec_open2 fail");
           return;
        }
    }
    
    if (video_de_frame == NULL) {
        video_de_frame = av_frame_alloc();
    }
    
    if ((ret = avcodec_send_packet(video_de_ctx,inpacket)) < 0) {
        LOGD("video avcodec_send_packet fail %s",av_err2str(ret));
        releaseSources();
        return;
    }
    while (true) {
        // 从解码缓冲区接收解码后的数据
        if((ret = avcodec_receive_frame(video_de_ctx, video_de_frame)) < 0) {
            if (ret == AVERROR_EOF) {
                // 解码缓冲区结束了，那么也要flush编码缓冲区
                doEncodeVideo(NULL);
            }
            break;
        }
        
        // 如果需要格式转换 则在这里进行格式转换
        if (video_en_frame == NULL) {
            video_en_frame = get_video_frame(video_en_ctx->pix_fmt, video_en_ctx->width, video_en_ctx->height);
            if (video_en_frame == NULL) {
                LOGD("cannot create vdioe fram");
                releaseSources();
                return;
            }
        }
        
        if (video_need_convert) {
            // 分配像素转换上下文
            if (sws_ctx == NULL) {
                AVCodecParameters *inpar = inFmtCtx->streams[video_in_stream_index]->codecpar;
                int dst_width = video_en_ctx->width;
                int dst_height = video_en_ctx->height;
                enum AVPixelFormat dst_pix_fmt = video_en_ctx->pix_fmt;
                sws_ctx = sws_getContext(inpar->width, inpar->height, (enum AVPixelFormat)inpar->format,
                                         dst_width, dst_height, dst_pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
                if (!sws_ctx) {
                    LOGD("sws_getContext fail");
                    releaseSources();
                    return;
                }
            }
            
            // 进行转换;返回目标height
            ret = sws_scale(sws_ctx, video_de_frame->data, video_de_frame->linesize, 0, video_de_frame->height, video_en_frame->data, video_en_frame->linesize);
            if (ret < 0) {
                LOGD("sws_scale fail");
                releaseSources();
                return;
            }
        } else {
            // 直接拷贝即可
            av_frame_copy(video_en_frame, video_de_frame);
        }
        
        
        // 数据拷贝到用于编码的
        /** 遇到问题：warning, too many B-frames in a row
         *  分析原因：之前下面doEncodeVideo()函数传递的参数为video_de_frame,这个是解码之后直接得到的AVFrame，它本身就包含了
         *  相关和编码不符合的参数
         *  解决方案：重新创建一个AVFrame，将解码后得到的AVFrame的data数据拷贝过去。然后用这个作为编码的AVFrame
         */
        video_en_frame->pts = video_pts;
        video_pts++;
        doEncodeVideo(video_en_frame);
    }
}

/** 遇到问题：生成的MOV和MP4文件可以播放，但是看不到小的预览图
 */
void Transcode::doEncodeVideo(AVFrame *frame)
{
    int ret = 0;
    // 获得了解码后的数据，然后进行重新编码
    if ((ret = avcodec_send_frame(video_en_ctx, frame)) < 0) {
        LOGD("video encode avcodec_send_frame fail");
        return;
    }
    
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        if((ret = avcodec_receive_packet(video_en_ctx, pkt)) < 0) {
            break;
        }

        // 将编码后的数据写入文件
        /** 遇到问题：生成的文件帧率不对
         *  分析原因：当调用avformat_write_header()后，内部会改变输出流的time_base参数，而这里没有重新调整packet的pts,dts,duration的值
         *  解决方案：使用av_packet_rescale_ts重新调整packet的pts,dts,duration的值
         */
        AVStream *stream = ouFmtCtx->streams[video_ou_stream_index];
        av_packet_rescale_ts(pkt, video_en_ctx->time_base, stream->time_base);
        pkt->stream_index = video_ou_stream_index;

        doWrite(pkt, true);
    }
}

void Transcode::doDecodeAudio(AVPacket *packet)
{
    int ret = 0;
    if (audio_de_ctx == NULL) {
        AVCodec *codec = avcodec_find_decoder(src_audio_id);
        if (!codec) {
            LOGD("audio decoder not found");
            releaseSources();
            return;
        }
        audio_de_ctx = avcodec_alloc_context3(codec);
        if (!audio_de_ctx) {
            LOGD("audio decodec_ctx fail");
            releaseSources();
            return;
        }
        
        // 设置音频解码上下文参数;这里来自于源文件的拷贝
        if (avcodec_parameters_to_context(audio_de_ctx, inFmtCtx->streams[audio_in_stream_index]->codecpar) < 0) {
            LOGD("audio set decodec ctx fail");
            releaseSources();
            return;
        }
        
        if ((avcodec_open2(audio_de_ctx, codec, NULL)) < 0) {
            LOGD("audio avcodec_open2 fail");
            releaseSources();
            return;
        }
    }
    
    // 创建解码用的AVFrame
    if (!audio_de_frame) {
        audio_de_frame = av_frame_alloc();
    }

    if ((ret = avcodec_send_packet(audio_de_ctx, packet)) < 0) {
        LOGD("audio avcodec_send_packet fail %d",ret);
        releaseSources();
        return;
    }
    
    while (true) {
        if ((ret = avcodec_receive_frame(audio_de_ctx, audio_de_frame)) < 0) {
            if (ret == AVERROR_EOF) {
                LOGD("audio decode finish");
                // 解码缓冲区结束了，那么也要flush编码缓冲区
                doEncodeAudio(NULL);
            }
            break;
        }
        
        // 创建编码器用的AVFrame
        if (audio_en_frame == NULL) {
            audio_en_frame = get_audio_frame(audio_en_ctx->sample_fmt, audio_en_ctx->channel_layout, audio_en_ctx->sample_rate, audio_en_ctx->frame_size);
            if (audio_en_frame == NULL) {
                LOGD("can not create audio frame ");
                releaseSources();
                return;
            }
        }
        
        // 解码成功，然后再重新进行编码;
        // 为了避免数据污染，所以这里只需要要将解码后得到的AVFrame中data数据拷贝到编码用的audio_en_frame中，解码后的其它数据则丢弃
        int pts_num = 0;
        if (audio_need_convert) {
            
            if (!audio_init) {
                ret = av_samples_alloc_array_and_samples(&audio_buffer, audio_en_frame->linesize, audio_en_frame->channels, audio_en_frame->nb_samples, (enum AVSampleFormat)audio_en_frame->format, 0);
                audio_init = true;
                left_size = 0;
            }
            /** 遇到问题：当音频编码方式不一致时转码后无声音
             *  分析原因：因为每个编码方式对应的AVFrame中的nb_samples不一样，所以再进行编码前要进行AVFrame的转换
             *  解决方案：进行编码前先转换
             */
            bool first = true;
            while (true) {
                // 进行转换
                if (first) {
                    ret = swr_convert(swr_ctx, audio_buffer, audio_en_frame->nb_samples, (const uint8_t**)audio_de_frame->data, audio_de_frame->nb_samples);
                    if (ret < 0) {
                        LOGD("swr_convert() fail %d",ret);
                        releaseSources();
                        return;
                    }
                    first = false;
                    
                    int use = ret-left_size >= 0 ?ret - left_size:ret;
                    int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                    for (int ch=0; ch<audio_en_frame->channels; ch++) {
                        for (int i = 0; i<use; i++) {
                            audio_en_frame->data[ch][(i+left_size)*size] = audio_buffer[ch][i*size];
                            audio_en_frame->data[ch][(i+left_size)*size+1] = audio_buffer[ch][i*size+1];
                            audio_en_frame->data[ch][(i+left_size)*size+2] = audio_buffer[ch][i*size+2];
                            audio_en_frame->data[ch][(i+left_size)*size+3] = audio_buffer[ch][i*size+3];
                        }
                    }
                    // 编码
                    left_size += ret;
                    if (left_size >= audio_en_frame->nb_samples) {
                        left_size -= audio_en_frame->nb_samples;
                        // 编码
                        audio_en_frame->pts = av_rescale_q(audio_pts, (AVRational){1,audio_en_frame->sample_rate}, audio_en_ctx->time_base);
                        audio_pts += audio_en_frame->nb_samples;
                        doEncodeAudio(audio_en_frame);
                        
                        if (left_size > 0) {
                            int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                            for (int ch=0; ch<audio_en_frame->channels; ch++) {
                                for (int i = 0; i<left_size; i++) {
                                    audio_en_frame->data[ch][i*size] = audio_buffer[ch][(use+i)*size];
                                    audio_en_frame->data[ch][i*size+1] = audio_buffer[ch][(use+i)*size+1];
                                    audio_en_frame->data[ch][i*size+2] = audio_buffer[ch][(use+i)*size+2];
                                    audio_en_frame->data[ch][i*size+3] = audio_buffer[ch][(use+i)*size+3];
                                }
                            }
                        }
                    }
                    
                } else {
                    ret = swr_convert(swr_ctx, audio_buffer, audio_en_frame->nb_samples, NULL, 0);
                    if (ret < 0) {
                        LOGD("1 swr_convert() fail %d",ret);
                        releaseSources();
                        return;
                    }
                    int size = av_get_bytes_per_sample((enum AVSampleFormat)audio_en_frame->format);
                    for (int ch=0; ch<audio_en_frame->channels; ch++) {
                        for (int i = 0; i < ret && i+left_size < audio_en_frame->nb_samples; i++) {
                            audio_en_frame->data[ch][(left_size + i)*size] = audio_buffer[ch][i*size];
                            audio_en_frame->data[ch][(left_size + i)*size + 1] = audio_buffer[ch][i*size + 1];
                            audio_en_frame->data[ch][(left_size + i)*size + 2] = audio_buffer[ch][i*size + 2];
                            audio_en_frame->data[ch][(left_size + i)*size + 3] = audio_buffer[ch][i*size + 3];
                        }
                    }
                    left_size += ret;
                    if (left_size >= audio_en_frame->nb_samples) {
                        left_size -= audio_en_frame->nb_samples;
                        LOGD("多了一个编码");
                        // 编码
                        audio_en_frame->pts = av_rescale_q(audio_pts, (AVRational){1,audio_en_frame->sample_rate}, audio_en_ctx->time_base);
                        audio_pts += audio_en_frame->nb_samples;
                        doEncodeAudio(audio_en_frame);
                    } else {
                        break;
                    }
                }
            }
            
        } else {
            av_frame_copy(audio_en_frame, audio_de_frame);
            pts_num = audio_en_frame->nb_samples;
            /** 遇到问题：得到的文件播放时音画不同步
             *  分析原因：由于音频的AVFrame的pts没有设置对；pts是基于AVCodecContext的时间基的时间，所以pts的设置公式：
             *  音频：pts  = (timebase.den/sample_rate)*nb_samples*index；  index为当前第几个音频AVFrame(索引从0开始)，nb_samples为每个AVFrame中的采样数
             *  视频：pts = (timebase.den/fps)*index；
             *  解决方案：按照如下代码方式设置AVFrame的pts
            */
            audio_en_frame->pts = audio_pts++;    // 造成了音画不同步的问题
            audio_en_frame->pts = av_rescale_q(audio_pts, (AVRational){1,audio_en_frame->sample_rate}, audio_en_ctx->time_base);
            audio_pts += pts_num;
            doEncodeAudio(audio_en_frame);
        }
    }
}

void Transcode::doEncodeAudio(AVFrame *frame)
{
    int ret = 0;
    if ((ret = avcodec_send_frame(audio_en_ctx, frame)) < 0) {
        LOGD("audio avcodec_send_frame fail %s",av_err2str(ret));
        releaseSources();
        return;
    }
    
    
    while (true) {
        AVPacket *pkt = av_packet_alloc();
        if ((ret = avcodec_receive_packet(audio_en_ctx, pkt)) < 0) {
            break;
        }
        
        // 说明编码得到了一个完整的帧
        // 因为调用avformat_write_header()函数后内部会改变ouFmtCtx->streams[audio_ou_stream_index]->time_base的值，而
        // pkt是按照audio_en_ctx->time_base来进行编码的，所以这里写入文件之前要进行时间的转换
        AVStream *stream = ouFmtCtx->streams[audio_ou_stream_index];
        av_packet_rescale_ts(pkt, audio_en_ctx->time_base, stream->time_base);
        pkt->stream_index = audio_ou_stream_index;
        
        doWrite(pkt, false);
    }
}

void Transcode::doWrite(AVPacket *packet,bool isVideo)
{
    if (!packet) {
        LOGD("packet is null");
        return;
    }
    
    AVPacket *a_pkt = NULL;
    AVPacket *v_pkt = NULL;
    AVPacket *w_pkt = NULL;
    if (isVideo) {
        v_pkt = packet;
    } else {
        a_pkt = packet;
    }
    
    // 为了精确的比较时间，先缓存一点点数据
    if (last_video_pts == 0 && isVideo && videoCache.size() == 0) {
        videoCache.push_back(packet);
        last_video_pts = packet->pts;
        LOGD(" 还没有 %s 数据",audio_pts==0?"audio":"video");
        return;
    }
    
    if (last_audio_pts == 0 && !isVideo && audioCache.size() == 0) {
        audioCache.push_back(packet);
        last_audio_pts = packet->pts;
        LOGD(" 还没有 %s 数据",video_pts==0?"video":"audio");
        return;
    }
    
    if (videoCache.size() > 0) {    // 里面有缓存了数据
        v_pkt = videoCache.front();
        if (isVideo) {
            videoCache.push_back(packet);
        }
    }
    
    if (audioCache.size() > 0) {    // 里面有缓存了数据
        a_pkt = audioCache.front();
        if (!isVideo) {
            audioCache.push_back(packet);
        }
    }
    
    if (v_pkt) {
        last_video_pts = v_pkt->pts;
    }
    
    if (a_pkt) {
        last_audio_pts = a_pkt->pts;
    }
    
    
    AVRational tb;
    AVStream *a_stream = ouFmtCtx->streams[audio_ou_stream_index];
    AVStream *v_stream = ouFmtCtx->streams[video_ou_stream_index];
    if (a_pkt && v_pkt) {   // 两个都有 则进行时间的比较
        if (av_compare_ts(last_audio_pts,a_stream->time_base,last_video_pts,v_stream->time_base) <= 0) { // 视频在后
            w_pkt = a_pkt;
            if (audioCache.size() > 0) {
                vector<AVPacket*>::iterator begin = audioCache.begin();
                audioCache.erase(begin);
            }
            tb = a_stream->time_base;
        } else {
            w_pkt = v_pkt;
            if (videoCache.size() > 0) {
                vector<AVPacket*>::iterator begin = videoCache.begin();
                videoCache.erase(begin);
            }
            tb = v_stream->time_base;
        }
        
    } else if (a_pkt) {
        w_pkt = a_pkt;
        if (audioCache.size() > 0) {
            vector<AVPacket*>::iterator begin = audioCache.begin();
            audioCache.erase(begin);
        }
        tb = a_stream->time_base;
    } else if (v_pkt) {
        w_pkt = v_pkt;
        if (videoCache.size() > 0) {
            vector<AVPacket*>::iterator begin = videoCache.begin();
            videoCache.erase(begin);
        }
        tb = v_stream->time_base;
    }
    
    static int sum = 0;
    if (!isVideo) {
        sum++;
    }
    LOGD("%s pts %d(%s) dts %d du %d a_size %d v_size %d sum %d",w_pkt->stream_index == audio_ou_stream_index?"audio":"video",w_pkt->pts,av_ts2timestr(w_pkt->pts,&tb),w_pkt->dts,w_pkt->duration,audioCache.size(),videoCache.size(),sum);
    if ((av_write_frame(ouFmtCtx, w_pkt)) < 0) {
        LOGD("av_write_frame fail");
    }
    
    av_packet_unref(w_pkt);
}

AVFrame* Transcode::get_audio_frame(enum AVSampleFormat smpfmt,int64_t ch_layout,int sample_rate,int nb_samples)
{
    AVFrame * audio_en_frame = av_frame_alloc();
    // 根据采样格式，采样率，声道类型以及采样数分配一个AVFrame
    audio_en_frame->sample_rate = sample_rate;
    audio_en_frame->format = smpfmt;
    audio_en_frame->channel_layout = ch_layout;
    audio_en_frame->nb_samples = nb_samples;
    int ret = 0;
    if ((ret = av_frame_get_buffer(audio_en_frame, 0)) < 0) {
        LOGD("audio get frame buffer fail %d",ret);
        return NULL;
    }

    if ((ret =  av_frame_make_writable(audio_en_frame)) < 0) {
        LOGD("audio av_frame_make_writable fail %d",ret);
        return NULL;
    }
    
    return audio_en_frame;
}

AVFrame* Transcode::get_video_frame(enum AVPixelFormat pixfmt,int width,int height)
{
    AVFrame *video_en_frame = av_frame_alloc();
    video_en_frame->format = pixfmt;
    video_en_frame->width = width;
    video_en_frame->height = height;
    int ret = 0;
    if ((ret = av_frame_get_buffer(video_en_frame, 0)) < 0) {
        LOGD("video get frame buffer fail %d",ret);
        return NULL;
    }

    if ((ret =  av_frame_make_writable(video_en_frame)) < 0) {
        LOGD("video av_frame_make_writable fail %d",ret);
        return NULL;
    }
    
    return video_en_frame;
}

void Transcode::releaseSources()
{
    if (inFmtCtx) {
        avformat_close_input(&inFmtCtx);
        inFmtCtx = NULL;
    }
    
    if (ouFmtCtx) {
        avformat_free_context(ouFmtCtx);
        ouFmtCtx = NULL;
    }
    
    if (video_en_ctx) {
        avcodec_free_context(&video_en_ctx);
        video_en_ctx = NULL;
    }
    
    if (audio_en_ctx) {
        avcodec_free_context(&audio_en_ctx);
        audio_en_ctx = NULL;
    }
    
    if (video_de_ctx) {
        avcodec_free_context(&video_de_ctx);
        video_de_ctx = NULL;
    }
    
    if (audio_de_ctx) {
        avcodec_free_context(&audio_de_ctx);
        audio_de_ctx = NULL;
    }
    
    if (video_de_frame) {
        av_frame_free(&video_de_frame);
        video_de_frame = NULL;
    }
    
    if (audio_de_frame) {
        av_frame_free(&audio_de_frame);
    }
    
    if (video_en_frame) {
        av_frame_free(&video_en_frame);
        video_en_frame = NULL;
    }
    
    if (audio_en_frame) {
        av_frame_free(&audio_en_frame);
        audio_en_frame = NULL;
    }
}
