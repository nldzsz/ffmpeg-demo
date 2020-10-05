//
//  demuxer.hpp
//  video_encode_decode
//
//  Created by apple on 2020/3/24.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef demuxer_hpp
#define demuxer_hpp

#include <stdio.h>
#include <string>
#include "CLog.h"
extern "C"{
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/file.h>
#include <libavformat/avio.h>
#include <libavutil/timestamp.h>
}
using namespace std;

/** ffmpeg编译完成后支持的解封装器位于libavformat目录下的demuxer_list.c文件中，具体的配置再.configure文件中，如下：
 *  print_enabled_components libavformat/demuxer_list.c AVInputFormat demuxer_list $DEMUXER_LIST
 *  xxxx.mp4对应的解封装器为ff_mov_demuxer
 */
class Demuxer
{
public:
    Demuxer();
    ~Demuxer();
    
    void doDemuxer(string srcpath);
};
#endif /* demuxer_hpp */
