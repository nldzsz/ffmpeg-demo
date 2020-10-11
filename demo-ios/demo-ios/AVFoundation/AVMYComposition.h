//
//  AVComposition.h
//  demo-ios
//
//  Created by apple on 2020/10/10.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AVMYComposition : NSObject

/** 实现音视频合并功能
 *  1、要合并的视频时长大于任何一个音频的时长，有可能小于两段音频的时长
 *  2、以视频的时长为基准，如果两段音频的时长之和大于视频时长，则截取掉第二个音频的部分时间
 */
- (void)startMerge:(NSURL*)audioUrl audio2:(NSURL*)audioUrl2 videoUrl:(NSURL*)videoUrl dst:(NSURL*)dsturl;

/** 合并任意两个相同容器类型容器文件的功能；合并后的文件分辨率取最低的文件分辨率，像素格式及颜色范围取第一文件的。编码方式则
 */
- (void)mergeFile:(NSURL*)filstURL twoURL:(NSURL*)twoURL dst:(NSURL*)dsturl;
@end
