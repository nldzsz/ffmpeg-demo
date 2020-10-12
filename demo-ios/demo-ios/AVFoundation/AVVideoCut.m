//
//  AVVideoCut.m
//  demo-ios
//
//  Created by apple on 2020/10/11.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVVideoCut.h"
#import <AVFoundation/AVFoundation.h>

@implementation AVVideoCut
{
    dispatch_semaphore_t processSemaphore;
}

- (void)cutVideo:(NSURL*)srcURL dst:(NSURL*)dstURL
{
    processSemaphore = dispatch_semaphore_create(0);
    AVURLAsset *asset = [[AVURLAsset alloc] initWithURL:srcURL options:nil];
    [asset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
        NSError *error = nil;
        AVKeyValueStatus status = [asset statusOfValueForKey:@"tracks" error:&error];
        if (status != AVKeyValueStatusLoaded) {
            NSLog(@"loaded error %@",error);
            return;
        }
        
        [self processAsset:asset dst:dstURL];
        
        
    }];
    dispatch_semaphore_wait(processSemaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束了");
}

- (void)processAsset:(AVAsset*)asset dst:(NSURL*)dstURL
{
    // 获取容器中的音视频轨道对象
    NSArray *videotracks = [asset tracksWithMediaType:AVMediaTypeVideo];
    NSArray *audiotracks = [asset tracksWithMediaType:AVMediaTypeAudio];
    AVAssetTrack *videoTrack = videotracks?videotracks[0]:nil;
    AVAssetTrack *audioTrack = audiotracks?audiotracks[0]:nil;
    
    // 划定要截取的时间;这里选择的时间为5-15秒的视频
    CMTime start = CMTimeMake(5*1000, 1000);
    CMTime time = CMTimeMake(10*1000, 1000);
    CMTimeRange range = CMTimeRangeMake(start, time);
    
    // 创建组合对象
    AVMutableComposition *compostion = [AVMutableComposition composition];
    if (audioTrack) {
        // 添加组合音频轨道
        AVMutableCompositionTrack *audiocomtrack = [compostion addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
        NSError *error = nil;
        // 在音频轨道中选取指定的时间范围的音频插入到组合音频轨道中
        [audiocomtrack insertTimeRange:range ofTrack:audioTrack atTime:kCMTimeZero error:&error];
    }
    
    if (videoTrack) {
        AVMutableCompositionTrack *videocomtrack = [compostion addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
        NSError *error = nil;
        [videocomtrack insertTimeRange:range ofTrack:videoTrack atTime:kCMTimeZero error:&error];
    }
    
    // 执行合并
    if ([[NSFileManager defaultManager] fileExistsAtPath:dstURL.path]) {
        [[NSFileManager defaultManager] removeItemAtURL:dstURL error:nil];
    }
    
    // 执行组合对象中组合轨道的编辑任务
    AVAssetExportSession *extSession = [[AVAssetExportSession alloc] initWithAsset:compostion presetName:AVAssetExportPresetHighestQuality];
    extSession.outputURL = dstURL;
    extSession.outputFileType = AVFileTypeMPEG4;
    NSLog(@"开始编辑");
    [extSession exportAsynchronouslyWithCompletionHandler:^{
        if (extSession.status != AVAssetExportSessionStatusCompleted) {
            NSLog(@"编辑 error %@",extSession.error);
        }
        NSLog(@"编辑完毕");
        
        dispatch_semaphore_signal(self->processSemaphore);
    }];
}
@end
