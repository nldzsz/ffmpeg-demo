//
//  videoresample.cpp
//  video_encode_decode
//
//  Created by apple on 2020/3/24.
//  Copyright © 2020 apple. All rights reserved.
//

#include "videoresample.hpp"

VideoScale::VideoScale(){
    
}

VideoScale::~VideoScale()
{
    
}

const char* nameForOptionType(AVOptionType type) {
    const char *names[19] = {
        "AV_OPT_TYPE_FLAGS",
        "AV_OPT_TYPE_INT",
        "AV_OPT_TYPE_INT64",
        "AV_OPT_TYPE_DOUBLE",
        "AV_OPT_TYPE_FLOAT",
        "AV_OPT_TYPE_STRING",
        "AV_OPT_TYPE_RATIONAL",
        "AV_OPT_TYPE_BINARY",
        "AV_OPT_TYPE_DICT",
        "AV_OPT_TYPE_UINT64",
        "AV_OPT_TYPE_CONST",
        "AV_OPT_TYPE_IMAGE_SIZE",
        "AV_OPT_TYPE_PIXEL_FMT",
        "AV_OPT_TYPE_SAMPLE_FMT",
        "AV_OPT_TYPE_VIDEO_RATE",
        "AV_OPT_TYPE_DURATION",
        "AV_OPT_TYPE_COLOR",
        "AV_OPT_TYPE_CHANNEL_LAYOUT",
        "AV_OPT_TYPE_BOOL"
    };
    
    return names[type];
}


/** 视频像素格式存储方式
 *  YUV420P：三个plane，按照 YYYY........U........V..........分别存储于各个plane通道
 *  RGB24:一个plane，按照RGBRGB....顺序存储在一个planne中
 *  NV12:两个plane，一个存储YYYYYY，另一个按照UV的顺序交叉存储
 *  NV21:两个plane，一个存储YYYYYY，另一个按照VU的顺序交叉存储
 */
/** 播放yuv的ffplay 命令
 *  ffplay -i  test_320x180_nv12.yuv -f rawvideo --pixel_format nv12 -video_size 320x180 -framerate 50
 */
/** 遇到问题：源MP4文件视频的帧率为50fps，解码成yuv之后用ffplay播放rawvideo速度变慢
 *  原因分析：ffplay播放rawvideo的默认帧率为25fps;如上添加-framerate 50 代表以50的fps播放
 */
void VideoScale::doScale(string srcpath,string dstpath)
{
    enum AVPixelFormat src_pix_fmt = AV_PIX_FMT_YUV420P;
    enum AVPixelFormat dst_pix_fmt = AV_PIX_FMT_NV12;
    int src_w = 320,src_h = 180;
    int dst_w = 160,dst_h = 90;
    
    /** 1、因为视频的planner个数最多不超过4个，所以这里的src_data数组和linesize数组长度都是4
     *  2、linesize数组存储的是各个planner的宽，对应"视频的宽"，由于字节对齐可能会大于等于对应"视频的宽"
    */
    uint8_t *src_buffer[4],*dst_buffer[4];
    int     src_linesize[4],dst_linesize[4];
    const AVPixFmtDescriptor *pixfmtDest = av_pix_fmt_desc_get(src_pix_fmt);
    
    // 一、获取视频转换上下文
    /**
     * 方式一：
     *  1、参数1 2 3：转换源数据宽，高，像素格式
     *  2、参数4 5 6：转换后数据宽，高，像素格式
     *  3、参数7：如果进行缩放，所采用的的缩放算法
     *  4、参数8：转换源数据的 SwsFilter 参数
     *  5、参数9：转换后数据的 SwsFilter 参数
     *  6、参数10：转换缩放算法的相关参数 const double* 类型数组
     *  7、8 9 10三个参数一般传递NULL，即采用默认值即可
     *  8、SWS_BICUBIC性能比较好，SWS_FAST_BILINEAR在性能和速度之间有一个比好好的平衡，SWS_POINT的效果比较差。
     */
    SwsContext *swsCtx = NULL;
#if 1
    swsCtx = sws_getContext(src_w,src_h,src_pix_fmt,
                                dst_w,dst_h,dst_pix_fmt,
                                SWS_BICUBIC,
                                NULL,NULL,
                                NULL
                                );
    if (swsCtx == NULL) {
        LOGD("sws_getContext fail");
        return;
    }
#else
    // 方式二；此方式可以设置更多的转换属性，比如设置YUV的颜色取值范围是MPEG还是JPEG
    swsCtx = sws_alloc_context();
//    const AVOption *opt = NULL;
//    LOGD("输出 AVOption的所有值===>");
    /** 基于AVClass的结构体。
     *  1、所有基于AVClass的结构体都可以用av_opt_set_xxx()函数和av_opt_get_xxx()函数来设置和获取结构体对应的属性，注意：AVClass
     *  必须是该结构体的第一个属性。这个结构体的每一个属性是AVOption结构体，包含了属性名，属性值等等。存储于AVClass的
     *  const struct AVOption *option中
     *  2、ffmpeg的常用结构体都是基于AVClass的，比如AVCodec，SwsContext等等。这些结构体对应属性的属性名在各个目录的
     *  options.c中，比如SwsContext的就在libswscale目录下的options.c文件中定义，并且有默认值
     *  3、av_opt_next()可以遍历基于AVClass的结构体的属性对应的AVOption结构体
     */
//    while ((opt = av_opt_next(swsCtx, opt)) != NULL) {
//        LOGD("name:%s help:%s type %s",opt->name,opt->help,nameForOptionType(opt->type));
//    }
    
    /** 遇到问题：视频进行压缩后变形
     *  原因分析：因为下面dstw的参数写成了src_w导致变形，设置争取即可
     */
    av_opt_set_pixel_fmt(swsCtx,"src_format",src_pix_fmt,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx, "srcw", src_w, AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx,"srch",src_h,AV_OPT_SEARCH_CHILDREN);
    /** 设置源的AVColorRange
     *  AVCOL_RANGE_JPEG：以JPEG为代表的标准中，Y、U、V的取值范围都是0-255。FFmpeg中称之为“jpeg” 范围。
     *  AVCOL_RANGE_MPEG：一般用于以Rec.601为代表（还包括BT.709 / BT.2020）的广播电视标准Y的取值范围是16-235，
     *  U、V的取值范围是16-240。FFmpeg中称之为“mpeg”范围。
     *  默认是MPEG范围，对于源来说 要与实际情况一致，这里源的实际范围是MPEG范围
     */
    av_opt_set_int(swsCtx,"src_range",0,AV_OPT_SEARCH_CHILDREN);
    
    av_opt_set_pixel_fmt(swsCtx,"dst_format",dst_pix_fmt,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx,"dstw",dst_w,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx,"dsth",dst_h,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx,"dst_range",1,AV_OPT_SEARCH_CHILDREN);
    av_opt_set_int(swsCtx,"sws_flags",SWS_BICUBIC,AV_OPT_SEARCH_CHILDREN);
    if (sws_init_context(swsCtx, NULL, NULL) < 0) {
        LOGD("sws_init_context fail");
        sws_freeContext(swsCtx);
        return;
    }
#endif
    
    LOGD("src_fmt %s dst_fmt %s",av_get_pix_fmt_name(src_pix_fmt),av_get_pix_fmt_name(dst_pix_fmt));
    
    // 二、分配内存 转换前和转换后的
    /** av_image_alloc()返回分配内存的大小
    *  1、该大小大于等于视频数据实际占用的大小，当最后一个参数为1时或者视频长宽刚好满足字节对齐时
    * 两者会相等。
    *  2、linesize对应视频的宽，它的值会大于等于视频的宽，在将视频数据读入这个内存或者从这个内存取出数据时都要用实际的宽的值
    *  3、该函数分配的内存实际上一块连续的内存，只不过src_data数组的指针分别指向了这块连续内存上对应各个planner
    *  上的首地址
    */
    int src_size = av_image_alloc(src_buffer,src_linesize,src_w,src_h,src_pix_fmt,16);
    LOGD("size %d linesize[0] %d linesize[1] %d linesize[2] %d log2_chroma_h %d",src_size,src_linesize[0],src_linesize[1],src_linesize[2],pixfmtDest->log2_chroma_h);
    if (src_size < 0) {
        LOGD("src image_alloc <0 %d",src_size);
        return;
    }
    int dst_size = av_image_alloc(dst_buffer,dst_linesize,dst_w,dst_h,dst_pix_fmt,16);
    if (dst_size < 0) {
        LOGD("dst_size < 0 %d",dst_size);
        return;
    }
    
    // 三、进行转换
    FILE *srcFile = fopen(srcpath.c_str(), "rb");
    if (srcFile == NULL) {
        LOGD("srcFile is NULL");
        return;
    }
    FILE *dstFile = fopen(dstpath.c_str(), "wb+");
    if (dstFile == NULL) {
        LOGD("dstFile is NULL");
        return;
    }
    
    while (true) {
        
        // 读取YUV420P的方式;注意这里要用src_w而非linesize中的值；yuv中uv的宽高为实际宽高的一半
        if (src_pix_fmt == AV_PIX_FMT_YUV420P) {
            size_t size_y = fread(src_buffer[0],1,src_w*src_h,srcFile);
            size_t size_u = fread(src_buffer[1],1,src_w/2*src_h/2,srcFile);
            size_t size_v = fread(src_buffer[2],1,src_w/2*src_h/2,srcFile);
            if (size_y <= 0 || size_u <= 0 || size_v <= 0) {
                LOGD("size_y %d size_u %d size_v %d",size_y,size_u,size_v);
                break;
            }
        } else if (src_pix_fmt == AV_PIX_FMT_RGB24) {
            size_t size = fread(src_buffer[0],1,src_w*src_h*3,srcFile);
            if (size <= 0) {
                LOGD("size %d",size);
                break;
            }
        } else if (src_pix_fmt == AV_PIX_FMT_NV12) {
            size_t size_y = fread(src_buffer[0],1,src_w * src_h,srcFile);
            size_t size_uv = fread(src_buffer[1],1,src_w * src_h/2,srcFile);
            if (size_y <= 0 || size_uv <= 0) {
                LOGD("size_y %d size_uv %d",size_y,size_uv);
                break;
            }
        } else if (src_pix_fmt == AV_PIX_FMT_NV21) {
            size_t size_y = fread(src_buffer[0],1,src_w * src_h,srcFile);
            size_t size_vu = fread(src_buffer[1],1,src_w * src_h/2,srcFile);
            if (size_y <= 0 || size_vu <= 0) {
                LOGD("size_y %d size_vu %d",size_y,size_vu);
                break;
            }
        }
        
        /** 参数1：转换上下文
        *  参数2：转换源数据内存地址
        *  参数3：转换源的linesize数组
        *  参数4：从源数据的撒位置起开始处理数据，一般是处理整个数据，传0
        *  参数5：换换源数据的height(视频的高)
        *  参数6：转换目的数据内存地址
        *  参数7：转换目的linesize数组
        *  返回：转换后的目的数据的height(该数据等于目标视频的高)
        */
        int size = sws_scale(swsCtx,src_buffer,src_linesize,0,src_h,
                  dst_buffer,dst_linesize);
        static int sum=0;
        sum++;
        LOGD("size == %d sum %d",size,sum);
        
        if (dst_pix_fmt == AV_PIX_FMT_YUV420P) {
            fwrite(dst_buffer[0],1,dst_w*dst_h,dstFile);
            fwrite(dst_buffer[1],1,dst_w/2*dst_h/2,dstFile);
            fwrite(dst_buffer[2],1,dst_w/2*dst_h/2,dstFile);
        } else if (dst_pix_fmt == AV_PIX_FMT_RGB24) {
            fwrite(dst_buffer[0],1,dst_w*dst_h*3,dstFile);
        } else if (dst_pix_fmt == AV_PIX_FMT_NV12) {
            fwrite(dst_buffer[0],1,dst_w*dst_h,dstFile);
            fwrite(dst_buffer[1],1,dst_w*dst_h/2,dstFile);
        } else if (dst_pix_fmt == AV_PIX_FMT_NV21) {
           fwrite(dst_buffer[0],1,dst_w*dst_h,dstFile);
           fwrite(dst_buffer[1],1,dst_w*dst_h/2,dstFile);
        }
    }
    
    
    sws_freeContext(swsCtx);
    fclose(srcFile);
    fclose(dstFile);
    av_freep(&src_buffer[0]);
    av_freep(&dst_buffer[0]);
}
