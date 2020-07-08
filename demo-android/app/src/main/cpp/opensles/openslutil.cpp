//
// Created by 飞拍科技 on 2019/5/25.
//
#include "openslutil.h"

/** opensl es支持的采样率范围为8000-192000
 * 传递给opensl es的采样率最好不要直接用数值表示，转换成opensl对应的枚举值
 * */
SLuint32 getSampleRate(Sample_rate sample_rate)
{
    SLuint32 sr = SL_SAMPLINGRATE_44_1;
    switch(sample_rate) {
        case 8000:
            sr = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            sr = SL_SAMPLINGRATE_11_025;
            break;
        case 16000:
            sr = SL_SAMPLINGRATE_16;
            break;
        case 22050:
            sr = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            sr = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            sr = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            sr = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            sr = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            sr = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            sr = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            sr = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            sr = SL_SAMPLINGRATE_192;
            break;
        default:
            break;
    }

    return sr;
}

SLuint32 getChannel_layout_Channels(Channel_Layout type)
{
    if (type == Channel_Layout_Mono) {
        return 1;
    } else if(type == Channel_Layout_steoro){
        return 2;
    }

    return 2;
}

SLuint32 getChannel_layout_Type(Channel_Layout type)
{
    if (type == Channel_Layout_Mono) {
        return SL_SPEAKER_FRONT_CENTER; // 单声道
    } else if(type == Channel_Layout_steoro){
        return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;  // 双声道 前左前右
    }

    return SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;;
}

SLuint32 getPCMSample_format(Sample_format format)
{
    if (format == Sample_format_SignedInteger_8){
        return SL_PCMSAMPLEFORMAT_FIXED_8;
    } else if(format == Sample_format_SignedInteger_16) {
        return SL_PCMSAMPLEFORMAT_FIXED_16;
    } else if(format == Sample_format_SignedInteger_32) {
        return SL_PCMSAMPLEFORMAT_FIXED_32;
    }

    return SL_PCMSAMPLEFORMAT_FIXED_16;
}
