//
//  CPPLog.h
//  study
//
//  Created by 飞拍科技 on 2018/12/29.
//  Copyright © 2018 飞拍科技. All rights reserved.
//

#ifndef CPPLog_h
#define CPPLog_h

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#ifdef __cplusplus

extern "C" {
#endif

// 输出当前时间的字符串 格式:YYYY-MM-DD HH:MM:SS
extern char* getTimeFormatForDebug(void);

// 开启调试日志
extern void enableDebug(void);
// 关闭调试 默认关闭
extern void disableDebug(void);
// 设置日志调试级别 默认Debug
//extern static int staticDebugLevelDebug;    // extern 和static 不能同时使用
extern int staticDebugLevelDebug;
extern int staticDebugLevelInfo;
extern int staticDebugLevelWaring;
extern int staticDebugLevelError;
extern void setDebuglevel(int level);

// 输出debug 日志
extern void LOGD(const char* format,...);
extern void LOGI(const char* format,...);
extern void LOGW(const char* format,...);
extern void LOGE(const char* format,...);

// 格式转化
extern void printUint32toHex(uint32_t val);
extern void printUint16toHex(uint16_t val);
extern void printUint8toHex(uint8_t val);

// 打印二进制串
void printHex(uint8_t* buff,int len);
    
#ifdef __cplusplus
} // extern "C"
#endif


#endif /* CPPLog_h */
