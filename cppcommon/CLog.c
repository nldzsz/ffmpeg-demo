//
//  CPPLog.cpp
//  study
//
//  Created by 飞拍科技 on 2019/1/2.
//  Copyright © 2019 飞拍科技. All rights reserved.
//

#include "CLog.h"

#define LOG_TAG "CLog"
// ===============日志定义 开始==================//
/**
 *  ## 表示字符串连接符号，##__VA_ARGS__ 表示将__VA_ARGS__与前面的字符串连接成一个字符串
 *  #  表示为后面的字符串添加双引号 比如 #A 则代表 "A"
 *  下面的fmt，是格式化的输出字符串比如"%s %d"，...表示可变参数，后面的__VA_ARGS__与前面可变参数对应
 */
#ifdef ANDROID
    #include <android/log.h>
    #define ILOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
    #define ILOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
    #define ILOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
    #define ILOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
    #define ILOGD(fmt, ...) printf(" %s " fmt "\n",getTimeFormatForDebug(), ##__VA_ARGS__)
    #define ILOGI(fmt, ...) printf(" %s " fmt "\n",getTimeFormatForDebug(), ##__VA_ARGS__)
    #define ILOGW(fmt, ...) printf(" %s " fmt "\n",getTimeFormatForDebug(), ##__VA_ARGS__)
    #define ILOGE(fmt, ...) printf(" %s " fmt "\n",getTimeFormatForDebug(), ##__VA_ARGS__)
#endif
// ===============日志定义 结束==================//

int staticEnableDebug = 1;
int staticDebugLevelDebug    = 0;
int staticDebugLevelInfo     = 1;
int staticDebugLevelWaring   = 2;
int staticDebugLevelError    = 3;
int staticDebugLevel         = 0;

// 开启调试
void enableDebug(void)
{
    staticEnableDebug = 1;
}

void disableDebug(void)
{
    staticEnableDebug = 0;
}

void setDebuglevel(int level)
{
    staticDebugLevel = level;
}

// 输出debug 日志
void LOGD(const char* format,...)
{
    if (staticEnableDebug==1 && staticDebugLevel<=staticDebugLevelDebug) {
        int len = 200;
        char s[len];
        va_list argp;
        int n=0;
        va_start(argp, format); //获得可变参数列表
        n=vsnprintf (s, len, format, argp); //写入字符串s
        va_end(argp); //释放资源
        
        ILOGD("Debug:%s",s);
    }
}

void LOGI(const char* format,...)
{
    if (staticEnableDebug==1 && staticDebugLevel<=staticDebugLevelInfo) {
        char s[300];
        va_list argp;
        int n=0;
        va_start(argp, format); //获得可变参数列表
        n=vsnprintf (s, 300, format, argp); //写入字符串s
        va_end(argp); //释放资源
        
        ILOGD("Info:%s",s);
    }
}

void LOGW(const char* format,...)
{
    if (staticEnableDebug==1 && staticDebugLevel<=staticDebugLevelWaring) {
        char s[300];
        va_list argp;
        int n=0;
        va_start(argp, format); //获得可变参数列表
        n=vsnprintf (s, 300, format, argp); //写入字符串s
        va_end(argp); //释放资源
        
        ILOGD("Waring:%s",s);
    }
}

void LOGE(const char* format,...)
{
    if (staticEnableDebug==1 && staticDebugLevel<=staticDebugLevelError) {
        char s[300];
        va_list argp;
        int n=0;
        va_start(argp, format); //获得可变参数列表
        n=vsnprintf (s, 300, format, argp); //写入字符串s
        va_end(argp); //释放资源
        
        ILOGD("Error:%s",s);
    }
}

char* getTimeFormatForDebug()
{
    struct timeval tv;
    struct tm * tm_local = NULL;
    gettimeofday(&tv,NULL);
#if __ANDROID__
    tm_local = gettimeofday(&tv.tv_sec,NULL);
#else
    tm_local = localtime(&tv.tv_sec);
#endif
    char str_f_t [30];
    strftime(str_f_t, sizeof(str_f_t), "%G-%m-%d %H:%M:%S", tm_local);
    char *returnStr = (char *)malloc(40);
    sprintf(returnStr, "%s:%.0f",str_f_t,tv.tv_usec/1000.0);
    
    return returnStr;
}

void printUint32toHex(uint32_t val)
{
    LOGD("%x %x %x %x",val&0xff,val>>8&0xff,val>>16&0xff,val>>24&0xff);
}

void printUint16toHex(uint16_t val)
{
    LOGD("%x %x",val&0xff,val>>8&0xff);
}

void printUint8toHex(uint8_t val)
{
    LOGD("%x",val&0xff);
}

void printHex(uint8_t* buff,int len)
{
    int  i;
//    char str[293] = {0};
    char str[10000] = {0};
    int lo = 4;
    float tt = len;
    int loop = len + (int)ceilf(tt/(lo));
    for( i = 0; i < loop; i++ ) {
        if (i % (lo+1) == 0) {
            str[i * 2] = 0x20;
            str[i * 2 + 1] = 0x20;
        } else {
            sprintf(&str[i * 2], "%02X", (unsigned char) buff[i]);
        }
    }

    LOGD("\n%s",str);
}
