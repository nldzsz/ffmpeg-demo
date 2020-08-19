//
//  HttpsTest.cpp
//  demo-mac
//
//  Created by apple on 2020/8/10.
//  Copyright © 2020 apple. All rights reserved.
//

#include "HttpsTest.hpp"

HttpsTest::HttpsTest()
{
    
}

HttpsTest::~HttpsTest()
{
    
}

void HttpsTest::doCopyStreamFromHttps(string srcpath, string dstpath)
{
    // 初始化网络
    avformat_network_init();
    
    AVFormatContext *in_fmtctx = NULL;
    int in_video_index = -1,in_audio_index = -1;
    int ret = 0;
    if ((ret = avformat_open_input(&in_fmtctx, srcpath.c_str(), NULL, NULL)) < 0) {
        printf("avformat_open_input failed %d",ret);
        return;
    }
    if((ret = avformat_find_stream_info(in_fmtctx, NULL)) < 0) {
        printf("avformat_find_stream_info failed %d",ret);
        return;
    }
    
    AVPacket *in_pkt = av_packet_alloc();
    while (av_read_frame(in_fmtctx, in_pkt) == 0) {
        printf("in_pkt pts %lld(%s)",in_pkt->pts,av_ts2timestr(in_pkt->pts,&in_fmtctx->streams[in_pkt->stream_index]->time_base));
    }
    
    
}
