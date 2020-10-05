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
@end
