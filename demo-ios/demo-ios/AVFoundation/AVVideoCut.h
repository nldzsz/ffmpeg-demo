//
//  AVVideoCut.h
//  demo-ios
//
//  Created by apple on 2020/10/11.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AVVideoCut : NSObject

/** 实现功能：截取音视频文件中指定范围段的内容
 */
- (void)cutVideo:(NSURL*)srcURL dst:(NSURL*)dstURL;
@end
