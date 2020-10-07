//
//  AVMuxer.h
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>


@interface AVMuxer : NSObject

/** 实现功能：接封装MP4文件然后再重新封装到MP4文件中，不重新进行编码
 */
- (void)remuxer:(NSURL*)srcURL dstURL:(NSURL*)dstURL;

/** 实现功能：将一个MP4文件转换成MOV文件
 *  视频编码方式由H264变成H265
 *  // 备注：ios 不支持mp3的编码
 *  // 备注：低端机型不支持H265编码
 */
- (void)transcodec:(NSURL*)srcURL dstURL:(NSURL*)dstURL;
@end
