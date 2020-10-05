//
//  demuxer.cpp
//  video_encode_decode
//
//  Created by apple on 2020/3/24.
//  Copyright © 2020 apple. All rights reserved.
//

#include "demuxer.hpp"

struct buffer_data {
    uint8_t *ptr;
    uint8_t *ptr_start;
    size_t size; ///< buffer size
};

Demuxer::Demuxer()
{
    
}

Demuxer::~Demuxer()
{
    
    
}

/** 参考ffmpeg源码file.c的file_seek方法。
 *  1、该函数的作用有两个，第一个返回外部缓冲区的大小，类似于lstat()函数；第二个设置外部缓冲读取指针的位置(读取指针的位置
 *  相对于缓冲区首地址来说的)，类似于lseek()函数
 *  2、AVSEEK_SIZE 代表返回外部缓冲区大小
 *  3、SEEK_CUR 代表将外部缓冲区读取指针从目前位置偏移offset
 *  4、SEEK_SET 代表将外部缓冲区读取指针设置到offset指定的偏移
 *  5、SEEK_END 代表将外部缓冲区读取指针设置到相对于尾部地址的偏移
 *  6、3/4/5情况时返回当前指针位置相对于缓冲区首地址的偏移。offset 的值可以为负数和0。
 */
static int64_t io_seek(void* opaque,int64_t offset,int whence)
{
    struct buffer_data *bd = (struct buffer_data*)opaque;
    if (whence == AVSEEK_SIZE) {
        return bd->size;
    }
    
    if (whence == SEEK_CUR) {
        bd->ptr += offset;
    } else if (whence == SEEK_SET) {
        bd->ptr = bd->ptr_start+offset;
    } else if (whence == SEEK_END) {
        bd->ptr = bd->ptr_start + bd->size + offset;
    }
    
    return (int64_t)(bd->ptr - bd->ptr_start);
}

/** 参考ffmpeg file.c的file_read()源码
 *  1、该函数的意思就是需要从外部读取指定大小buf_size的数据到指定的buf中；这里外部是一个内存缓存
 *  2、每次读取完数据后需要将读取指针后移
 *  3、如果外部数据读取完毕，则需要返回AVERROR_EOF错误
 *  4、读取成功，返回实际读取的字节数
 *  5、如果没有数据直接 return 0; 那么将提示"Invalid return value 0 for stream protocol"，并且avformat_open_input()会初始化失败
 */
/** 遇到问题：提示"Invalid return value 0 for stream protocol"
 *  分析原因：avformat_open_input()函数内部在调用readFunc()读取输入流数据分析格式时，readFunc()由于没有数据直接return 0；导致avformat_open_input()解析
 *  失败，所以要想正确的解析出输入源的码流格式，必须有足够的输入数据才可。
 *  解决方案：给足够的数据用以成功解析
*/
static int io_read(void *opaque, uint8_t *buf, int buf_size)
{
    static int total = 0;
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, (int)(bd->ptr_start+bd->size-bd->ptr));
    total += buf_size;
    if (buf_size <= 0)
        return AVERROR_EOF;
//    LOGD("ptr:%p size:%zu buf_size %d total %d\n", bd->ptr, bd->size,buf_size,total);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;

    return buf_size;
}

void Demuxer::doDemuxer(string srcPath)
{
    AVFormatContext *inFmtCtx = NULL;
    int ret = 0;
    struct timeval btime;
    struct timeval etime;
    gettimeofday(&btime, NULL);
#define Use_Custom_io   0
#if Use_Custom_io
    AVIOContext *ioCtx;
    uint8_t *io_ctx_buffer = NULL,*buffer = NULL;
    size_t io_ctx_buffer_size = 4096,buffer_size;
    buffer_data bd = {0};
    ret = av_file_map(srcPath.c_str(),&buffer,&buffer_size,0,NULL);
    if (ret < 0) {
        LOGD("av_file_map fail");
        return;
    }
    bd.ptr = buffer;
    bd.ptr_start = buffer;
    bd.size = buffer_size;
    
    inFmtCtx = avformat_alloc_context();
    if (inFmtCtx == NULL) {
        LOGD("avformat_alloc_context fail");
        return;
    }
    io_ctx_buffer = (uint8_t*)av_mallocz(io_ctx_buffer_size);
    /** 遇到问题：如果没有指定io_seek函数，对于MP4文件来说，如果mdata在moov标签的后面，采用自定义的AVIOContext时
     *  候avformat_find_stream_info() 返回"Could not findcodec parameters for stream 0 ........:
     *  unspecified pixel formatConsider increasing the value for the 'analyzeduration' and 'probesize' options的错误
     *  av_read_frame()返回Invalid data found when processing input的错误
     *  分析原因：在创建AVIOContext时没有指定seek函数
     *  解决方案：因为创建AVIOContext时没有指定io_seek函数并正确实现io_read()和io_seek()相关逻辑；参考如上io_seek和io_read函数
     */
    ioCtx = avio_alloc_context(io_ctx_buffer,(int)io_ctx_buffer_size,0,&bd,&io_read,NULL,&io_seek);
    if (ioCtx == NULL) {
        LOGD("avio_alloc_context fail");
        return;
    }
    inFmtCtx->pb = ioCtx;
#endif
    /** 遇到问题：avformat_open_input -1094995529
     *  原因分析：这里要打开的文件是MP4文件，而对应的MP4的解封装器没有编译到ffmpeg中，--enable-demuxer=mov打开重新编译即可。
     */
    /** 参数1：AVFormatContext 指针变量，可以用avformat_alloc_context()先初始化或者直接初始化为NULL
     *  参数2：文件名路径
     *  参数3：AVInputFormat，接封装器对象，传NULL，则根据文件名后缀猜测。非NULL，则由这个指定的AVInputFormat进行解封装
     *  参数4：解封装相关参数，传NULL用默认即可
     */
    ret = avformat_open_input(&inFmtCtx,srcPath.c_str(),NULL,NULL);
    if (ret < 0) {
        LOGD("avformat_open_input fail %d error:%s",ret,av_err2str(ret));
        return;
    }
    
    LOGD("ddd probesize %d analyzeduration %d",inFmtCtx->probesize,inFmtCtx->max_analyze_duration);
    ret = avformat_find_stream_info(inFmtCtx,NULL);
    if (ret < 0) {
        LOGD("avformat_find_stream_info fail %d error:%s",ret,av_err2str(ret));
        return;
    }
    LOGD("begin av_dump_format");
    av_dump_format(inFmtCtx,0,NULL,0);
    LOGD("end av_dump_format");
    
    // 解析出封装格式中的标签
    LOGD("begin mediadata \n\n");
    const AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(inFmtCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        LOGD("tag key:%s value:%s",tag->key,tag->value);
    }
    LOGD("end mediadata \n\n");
    
    // 解析出封装格式中的编码相关参数
    int videoStream_Index = -1;
    int audioStream_Index = -1;
    for (int i = 0;i<inFmtCtx->nb_streams;i++) {
        AVStream *stream = inFmtCtx->streams[i];
        enum AVCodecID cId = stream->codecpar->codec_id;
        int format = stream->codecpar->format;
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream_Index = i;
            LOGD("begin video AVStream \n\n");
            LOGD("code_id %s format ",avcodec_get_name(cId));
            
            const AVPixFmtDescriptor *fmtDes = av_pix_fmt_desc_get((enum AVPixelFormat)format);
            LOGD("AVPixFmtDescriptor name %s",fmtDes->name);
            
            LOGD("width %d height %d",stream->codecpar->width,stream->codecpar->height);
            /** 颜色的取值范围
             *  AVCOL_RANGE_MPEG:代表以BT601,BT709，BT2020等以广播电视系统类的颜色取值范围类型
             *  AVCOL_RANGE_JPEG:代表以电脑等显示器类的颜色取值范围
             */
            LOGD("color_range %d",stream->codecpar->color_range);
            /** 所采用的颜色空间，比如RGB,YUV,CMYK等等
             */
            LOGD("color_space %s",av_get_colorspace_name(stream->codecpar->color_space));
            LOGD("video_delay %d\n\n",stream->codecpar->video_delay);
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream_Index = i;
            LOGD("begin audio AVStream \n\n");
            LOGD("code_id %s format ",avcodec_get_name(cId));
            LOGD("samplefmt %s",av_get_sample_fmt_name((enum AVSampleFormat)format));
            LOGD("channel_layout %s channels %d",av_get_channel_name(stream->codecpar->channel_layout),stream->codecpar->channels);
            LOGD("sample_rate %d",stream->codecpar->sample_rate);
            LOGD("frame_size %d",stream->codecpar->frame_size);
            
        } else {
            LOGD("other type");
        }
    }
    LOGD("end AVStream\n\n");
    
    
    AVPacket *packet = av_packet_alloc();
    static int anum = 0;
    static int vnum = 0;
    LOGD("begin av_read_frame");
    while ((ret = av_read_frame(inFmtCtx, packet)) >= 0) {
        
        if (videoStream_Index == packet->stream_index) {
            vnum++;
            LOGD("video size %d num %d pos %ld pts(%s)",packet->size,vnum,packet->pos,av_ts2timestr(packet->pts, &(inFmtCtx->streams[packet->stream_index]->time_base)));
        } else if (audioStream_Index == packet->stream_index){
            anum++;
            LOGD("audio size %d num %d pos %ld pts(%s)",packet->size,anum,packet->pos,av_ts2timestr(packet->pts, &(inFmtCtx->streams[packet->stream_index]->time_base)));
        } else {
            LOGD("other size %d pos %ld",packet->size,packet->pos);
        }
        
        av_packet_unref(packet);
    }
    LOGD("end av_read_frame ret %s",av_err2str(ret));
    gettimeofday(&etime, NULL);
    LOGD("解析耗时 %.2fs",(etime.tv_sec - btime.tv_sec) + (etime.tv_usec - btime.tv_usec)/1000000.0f);
    
    /** 释放内存
     */
    avformat_close_input(&inFmtCtx);
    // 对于自定义的AVIOContext，先释放里面的buffer，在释放AVIOContext对象
#if Use_Custom_io
    if (ioCtx) {
        av_freep(&ioCtx->buffer);
    }
    avio_context_free(&ioCtx);
    av_file_unmap(buffer, buffer_size);
#endif
}
