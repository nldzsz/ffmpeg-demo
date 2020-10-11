//
//  AVComposition.m
//  demo-ios
//
//  Created by apple on 2020/10/10.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVMYComposition.h"
#import <AVFoundation/AVFoundation.h>

@implementation AVMYComposition
{
    dispatch_semaphore_t processSemaphore;
}
- (void)startMerge:(NSURL*)audioUrl1 audio2:(NSURL*)audioUrl2 videoUrl:(NSURL*)videoUrl dst:(NSURL*)dsturl
{
    processSemaphore = dispatch_semaphore_create(0);
    
    /** AVMutableComposition对象
     * 组合对象，它是AVAsset的子类，通过它来实现音视频的合并。它就相当于一个编辑容器，每一个要被合并的
     * 音频或者视频轨道被组装为AVMutableCompositionTrack然后进行合并
     *
     *  AVMutableCompositionTrack组合对象轨道，他是AVAssetTrack的子类。代表了每一个要被合并的音频或者视频轨道
     */
    AVMutableComposition *mixComposition = [AVMutableComposition composition];
    
    // 添加一个组合对象轨道，用于添加视频轨道
    AVMutableCompositionTrack *videoCompostioTrack = [mixComposition addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
    AVURLAsset *videoAsset = [[AVURLAsset alloc] initWithURL:videoUrl options:nil];
    CMTime videoDuration = videoAsset.duration;
    AVAssetTrack *vdeotrack = [[videoAsset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
    CMTimeRange videoTiemRange = CMTimeRangeMake(kCMTimeZero, videoDuration);
    NSError *error = nil;
    [videoCompostioTrack insertTimeRange:videoTiemRange ofTrack:vdeotrack atTime:kCMTimeZero error:&error];
    if (error) {
        NSLog(@"video insert error %@",error);
        return;
    }
    
    // 添加一个组合对象轨道，第二个参数为kCMPersistentTrackID_Invalid代表由系统自动生成ID
    AVMutableCompositionTrack *audioComTrack = [mixComposition addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
    AVURLAsset *audioAsset1 = [AVURLAsset assetWithURL:audioUrl1];
    // 将同步解析，会阻塞当前线程
    CMTime duration1 = audioAsset1.duration;
    AVAssetTrack *audioTrack1 = [[audioAsset1 tracksWithMediaType:AVMediaTypeAudio] objectAtIndex:0];
    CMTimeRange firstTimeRange = CMTimeRangeMake(kCMTimeZero,duration1);
    // 往组合对象轨道中添加轨道对象
    [audioComTrack insertTimeRange:firstTimeRange ofTrack:audioTrack1 atTime:kCMTimeZero error:&error];
    if (error) {
        NSLog(@"audio track %@",error);
        return;
    }
   
    AVURLAsset *audioAsset2 = [AVURLAsset assetWithURL:audioUrl2];
    // 阻塞当前线程
    CMTime duration2 = audioAsset2.duration;
    CMTime newDuration2 = duration2;
    if (CMTimeGetSeconds(duration1)+CMTimeGetSeconds(duration2) > CMTimeGetSeconds(videoDuration) && CMTimeGetSeconds(duration1) < CMTimeGetSeconds(duration2)) {
        newDuration2 = CMTimeSubtract(videoDuration, duration1);
    }
    CMTimeRange secondTimeRange = CMTimeRangeMake(kCMTimeZero, newDuration2);
    NSLog(@" tt %f tt %f",CMTimeGetSeconds(duration1),CMTimeGetSeconds(newDuration2));
    AVAssetTrack *audioTrack2 = [[audioAsset2 tracksWithMediaType:AVMediaTypeAudio] objectAtIndex:0];
    /** 参数解释：
     *  timeRange：代表截取track的时间范围内容然后插入这个组合对象的轨道中
     *  startTime：代表将这段内容按组对象轨道时间轴的指定位置插入
     */
    [audioComTrack insertTimeRange:secondTimeRange ofTrack:audioTrack2 atTime:duration1 error:&error];
    
    // 执行合并
    if ([[NSFileManager defaultManager] fileExistsAtPath:dsturl.path]) {
        [[NSFileManager defaultManager] removeItemAtURL:dsturl error:nil];
    }
    
    // 合并导出会话
    AVAssetExportSession *exportSession = [[AVAssetExportSession alloc] initWithAsset:mixComposition presetName:AVAssetExportPresetHighestQuality];
    exportSession.outputURL = dsturl;
    exportSession.outputFileType = AVFileTypeQuickTimeMovie;
    
    [exportSession exportAsynchronouslyWithCompletionHandler:^{
        NSLog(@"over");
        dispatch_semaphore_signal(self->processSemaphore);
    }];
    
    dispatch_semaphore_wait(processSemaphore, DISPATCH_TIME_FOREVER);
    
    NSLog(@"结束了");
}


- (void)mergeFile:(NSURL*)filstURL twoURL:(NSURL*)twoURL dst:(NSURL*)dsturl
{
    NSLog(@"开始");
    processSemaphore = dispatch_semaphore_create(0);
    
    // 创建组合对象
    AVMutableComposition *composition = [AVMutableComposition composition];
    
    // 为组合对象添加组合对象音频轨道，用于合并所有音频
    AVMutableCompositionTrack *audioComTrack = [composition addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
    // 为组合对象添加组合对象视频轨道，用于合并所有视频
    AVMutableCompositionTrack *videoComTrack = [composition addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
    
    // 准备第一个容器文件的所有音频和视频轨道
    AVURLAsset *asset1 = [[AVURLAsset alloc] initWithURL:filstURL options:nil];
    NSArray *audioTracks1 = [asset1 tracksWithMediaType:AVMediaTypeAudio];
    NSArray *videoTracks1 = [asset1 tracksWithMediaType:AVMediaTypeVideo];
    AVAssetTrack *audioTrack1 = audioTracks1?audioTracks1[0]:nil;
    AVAssetTrack *videoTrack1 = videoTracks1?videoTracks1[0]:nil;
    
    
    // 准备第二个文件的所有音频和视频轨道
    AVURLAsset *asset2 = [[AVURLAsset alloc] initWithURL:twoURL options:nil];
    NSArray *audioTracks2 = [asset2 tracksWithMediaType:AVMediaTypeAudio];
    NSArray *videoTracks2 = [asset2 tracksWithMediaType:AVMediaTypeVideo];
    AVAssetTrack *audioTrack2 = audioTracks2?audioTracks2[0]:nil;
    AVAssetTrack *videoTrack2 = videoTracks2?videoTracks2[0]:nil;
    
    // 将解析出的每个文件的各个轨道添加到组合对象的对应的用于编辑的音视频轨道对象中
    CMTimeRange audioTmeRange1 = kCMTimeRangeZero;
    CMTimeRange videoTmeRange1 = kCMTimeRangeZero;
    NSError *error = nil;
    if (audioTrack1) {
        audioTmeRange1 = CMTimeRangeMake(kCMTimeZero, asset1.duration);
        // 组合对象音频轨道添加音频轨道
        [audioComTrack insertTimeRange:audioTmeRange1 ofTrack:audioTrack1 atTime:kCMTimeZero error:&error];
        if (error) {
            NSLog(@"audio1 error %@",error);
            return;;
        }
    }
    
    if (videoTrack1) {
        videoTmeRange1 = CMTimeRangeMake(kCMTimeZero, asset1.duration);
        // 组合对象音频轨道添加音频轨道
        [videoComTrack insertTimeRange:videoTmeRange1 ofTrack:videoTrack1 atTime:kCMTimeZero error:&error];
        if (error) {
            NSLog(@"video1 error %@",error);
            return;;
        }
    }
    
    // 处理第二个文件
    CMTimeRange audioTmeRange2 = kCMTimeRangeZero;
    CMTimeRange videoTmeRange2 = kCMTimeRangeZero;
    if (audioTrack2) {
        audioTmeRange2 = CMTimeRangeMake(kCMTimeZero, asset2.duration);
        [audioComTrack insertTimeRange:audioTmeRange2 ofTrack:audioTrack2 atTime:asset1.duration error:&error];
        if (error) {
            NSLog(@"audio2 error %@",error);
            return;;
        }
    }
    if (videoTrack2) {
        videoTmeRange2 = CMTimeRangeMake(kCMTimeZero, asset2.duration);
        [videoComTrack insertTimeRange:videoTmeRange2 ofTrack:videoTrack2 atTime:asset1.duration error:&error];
        if (error) {
            NSLog(@"video2 error %@",error);
            return;;
        }
    }
    
    // 执行合并
    if ([[NSFileManager defaultManager] fileExistsAtPath:dsturl.path]) {
        [[NSFileManager defaultManager] removeItemAtURL:dsturl error:nil];
    }
    
    // 执行合并轨道对象会话
    AVAssetExportSession *exportSession = [[AVAssetExportSession alloc] initWithAsset:composition presetName:AVAssetExportPresetHighestQuality];
    exportSession.outputURL = dsturl;
    exportSession.outputFileType = AVFileTypeMPEG4;
    
    [exportSession exportAsynchronouslyWithCompletionHandler:^{
        NSLog(@"over");
        if (exportSession.status != AVAssetExportSessionStatusCompleted) {
            NSLog(@"error %@",exportSession.error);
        }
        
        dispatch_semaphore_signal(self->processSemaphore);
    }];
    
    dispatch_semaphore_wait(self->processSemaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束了");
}
@end
