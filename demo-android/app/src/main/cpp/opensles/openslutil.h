//
// Created by 飞拍科技 on 2019/5/25.
//

#ifndef MEDIA_ANDROID_OPENSLUTIL_H
#define MEDIA_ANDROID_OPENSLUTIL_H

#include <SLES/OpenSLES.h>
/** 1、OpenSL ES 每个采样所占的bit数，有8位 16位 20位 24位 28位 32位。
 *  2、都是有符号类型
 *  3、为了处理方便，一般就用8 16 32三种
 * */
enum Sample_format_ {
    Sample_format_SignedInteger_8,
    Sample_format_SignedInteger_16,         // 每个采样用有符号16位表示
    Sample_format_SignedInteger_32          // 每个采样用有符号32位表示
};
typedef enum Sample_format_ Sample_format;

/** 声道类型
 * */
enum Channel_Layout_ {
    Channel_Layout_Mono,          // 单声道
    Channel_Layout_steoro         // 多声道
};
typedef enum Channel_Layout_  Channel_Layout;

/** 采样率
 * */
typedef int Sample_rate;

// 将数字采样率转化成openls es可识别的枚举值
SLuint32 getSampleRate(Sample_rate sample_rate);

// 根据声道类型获取声道数目
SLuint32 getChannel_layout_Channels(Channel_Layout type);

// 将声道类型转换为OPenSL ES的声道类型
SLuint32 getChannel_layout_Type(Channel_Layout type);

// 将采样格式类型转换为OPenSL ES的类型
SLuint32 getPCMSample_format(Sample_format format);

#endif //MEDIA_ANDROID_OPENSLUTIL_H
