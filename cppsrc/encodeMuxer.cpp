//
//  encodeMuxer.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/10.
//  Copyright © 2020 apple. All rights reserved.
//

#include "encodeMuxer.hpp"

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC

void EncodeMuxer::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGD("pts:%d pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           pkt->pts, av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

int EncodeMuxer::write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
void EncodeMuxer::add_video_stream(AVFormatContext *oc,AVCodec **codec,enum AVCodecID codec_id)
{
    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    // AVStream用于和具体的音/视频/字母等数据流关联;对于往文件写入数据，必须在avformat_write_header()函数前手动创建
    // 对于从文件中读取数据，其它函数内部自动创建
    video_ou_stream = avformat_new_stream(oc, NULL);
    if (!video_ou_stream) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    //
    video_ou_stream->id = oc->nb_streams-1;
    video_en_ctx = avcodec_alloc_context3(*codec);
    if (!video_en_ctx) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }

    video_en_ctx->codec_id = codec_id;

    video_en_ctx->bit_rate = 400000;
    /* Resolution must be a multiple of two. */
    video_en_ctx->width    = 352;
    video_en_ctx->height   = 288;
    /* timebase: This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identical to 1. */
    video_ou_stream->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
    video_en_ctx->time_base       = video_ou_stream->time_base;

    video_en_ctx->gop_size      = 12; /* emit one intra frame every twelve frames at most */
    video_en_ctx->pix_fmt       = STREAM_PIX_FMT;
    if (video_en_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B-frames */
        video_en_ctx->max_b_frames = 2;
    }
    if (video_en_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        video_en_ctx->mb_decision = 2;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        video_en_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

AVFrame* EncodeMuxer::alloc_audio_frame(enum AVSampleFormat sample_fmt,uint64_t channel_layout,int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}

/**************************************************************/
/* video output */

AVFrame* EncodeMuxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

void EncodeMuxer::open_video()
{
    int ret;
    AVCodecContext *c = video_en_ctx;

    /* open the codec */
    ret = avcodec_open2(c, video_codec,NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(video_ou_stream->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}

/* Prepare a dummy image. */
void EncodeMuxer::fill_yuv_image(AVFrame *pict, int frame_index,int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

AVFrame* EncodeMuxer::get_video_frame()
{
    AVCodecContext *c = video_en_ctx;

    /* check if we want to generate more frames */
    if (av_compare_ts(next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(frame) < 0)
        exit(1);

    fill_yuv_image(frame, next_pts, c->width, c->height);
    frame->pts = next_pts++;

    return frame;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
int EncodeMuxer::write_video_frame(AVFormatContext *oc)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    c = video_en_ctx;

    frame = get_video_frame();

    av_init_packet(&pkt);

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, video_ou_stream, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}

void EncodeMuxer::close_stream(AVFormatContext *oc)
{
    avcodec_free_context(&video_en_ctx);
    av_frame_free(&frame);
}

void EncodeMuxer::doEncodeMuxer(string dstPath)
{
    const char *filename = dstPath.c_str();
    /** 用于往文件写入数据的输出流
    */
    AVOutputFormat *fmt;
    int ret;
    int have_video = 0;
    int encode_video = 0;
    next_pts = 0;
    ouFmtCtx = NULL;
    video_codec = NULL;
    avformat_alloc_output_context2(&ouFmtCtx, NULL, NULL, filename);
    if (!ouFmtCtx) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&ouFmtCtx, NULL, "mpeg", filename);
    }
    if (!ouFmtCtx)
        return;
    fmt = ouFmtCtx->oformat;
    
    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_video_stream(ouFmtCtx, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    
    if (have_video)
        open_video();
    
    av_dump_format(ouFmtCtx, 0, filename, 1);
    
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        /** 往文件中写入数据的步骤：
         *  步骤1、将AVFormatContext中的AVIOContext上下文打开，并且和文件关联，这样就可以开始往文件中写入数据了
         */
        ret = avio_open(&ouFmtCtx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return;
        }
    }
    
    ret = avformat_write_header(ouFmtCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return;
    }
    
    
    while (encode_video) {
        // 如果视频数据pts在前面 则写入视频数据
        encode_video = !write_video_frame(ouFmtCtx);
    }
    
    av_write_trailer(ouFmtCtx);
}
