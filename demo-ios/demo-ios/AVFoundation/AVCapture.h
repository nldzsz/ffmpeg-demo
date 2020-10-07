//
//  AVCapture.h
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

@interface AVCapture : NSObject

// 录制一段时间的音视频然后保存到文件中，苹果只支持MOV mp4 m4v等少数格式
- (void)startCaptureToURL:(NSURL*)dstURL duration:(float)time fileType:(AVFileType)fileType;
@end
