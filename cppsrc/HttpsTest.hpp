//
//  HttpsTest.hpp
//  demo-mac
//
//  Created by apple on 2020/8/10.
//  Copyright © 2020 apple. All rights reserved.
//

#ifndef HttpsTest_hpp
#define HttpsTest_hpp

#include <stdio.h>
#include <string>

using namespace std;
extern "C" {
#include "Common.h"
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

class HttpsTest
{
public:
    HttpsTest();
    ~HttpsTest();
    /** 以流拷贝的方式从https协议地址下载文件到本地，如下载https://img.flypie.net/tec1-7.mp4的mp4的数据到本地https.mp4文件中
     */
    void doCopyStreamFromHttps(string srcpath,string dstpath);
};
#endif /* HttpsTest_hpp */
