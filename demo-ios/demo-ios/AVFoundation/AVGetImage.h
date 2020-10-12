//
//  AVGetImage.h
//  demo-ios
//
//  Created by apple on 2020/10/11.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AVGetImage : NSObject

/** 实现功能：
 *  从视频文件中截图指定时间段的视频帧所有视频帧，并最终以JPG的格式保存到指定目录中
 */
- (void)getImageFromURL:(NSURL*)srURL saveDstPath:(NSString*)dstPath;
@end
