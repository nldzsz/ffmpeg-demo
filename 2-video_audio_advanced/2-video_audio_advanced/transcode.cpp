//
//  transcode.cpp
//  2-video_audio_advanced
//
//  Created by apple on 2020/5/3.
//  Copyright © 2020 apple. All rights reserved.
//

#include "transcode.hpp"

/** 实现MP4转换成MOV,FLV,TS,AVI文件，不改变编码方式
 */
void Transcode::doTranscodeForMuxer()
{
    string curFile(__FILE__);
    unsigned long pos = curFile.find("2-video_audio_advanced");
    if (pos == string::npos) {
        LOGD("not find file");
        return;
    }
    string srcDic = curFile.substr(0,pos)+"filesources/";
    string srcPath = srcDic + "test_1280x720_3.mp4";
    string dstPath = srcDic + "2-test_1280x720_3.flv";   // 这里后缀可以改成flv,ts,avi 则生成对应的文件
    
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
             *  分析原因：之前的写法是没有加下面的判断条件直接为newStream->codecpar->codec_tag = 0;这样avi将选择默认的h264作为码流格式，导致两边不一样
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
//        LOGD("write packet index %d size %d",in_packet->stream_index,in_packet->size);
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
    
    av_write_trailer(ou_fmtCtx);
    avformat_close_input(&in_fmtCtx);
    avformat_free_context(ou_fmtCtx);
    
}
