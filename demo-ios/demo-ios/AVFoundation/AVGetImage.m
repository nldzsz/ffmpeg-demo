//
//  AVGetImage.m
//  demo-ios
//
//  Created by apple on 2020/10/11.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVGetImage.h"
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

@implementation AVGetImage
{
    // 视频处理队列信号量
    dispatch_semaphore_t processSemaphore;
}
- (void)getImageFromURL:(NSURL*)srURL saveDstPath:(NSString*)dstPath
{
    processSemaphore = dispatch_semaphore_create(0);
    
    AVURLAsset *asset = [[AVURLAsset alloc] initWithURL:srURL options:nil];
    // 从视频中提取视频帧同样适用于远程文件
//    asset = [[AVURLAsset alloc] initWithURL:[NSURL URLWithString:@"https://images.flypie.net/test_1280x720_3.mp4"] options:nil];
    [asset loadValuesAsynchronouslyForKeys:@[@"tracks",@"duration"] completionHandler:^{
        
        NSError *error = nil;
        AVKeyValueStatus status = [asset statusOfValueForKey:@"tracks" error:&error];
        if (status != AVKeyValueStatusLoaded) {
            NSLog(@"key value load error %@",error);
            return;
        }
        
        [self processAsset:asset dst:dstPath];
        
        dispatch_semaphore_signal(self->processSemaphore);
        
    }];
    
    dispatch_semaphore_wait(processSemaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束了。。。");
    
}

- (void)processAsset:(AVAsset*)asset dst:(NSString*)path
{
    NSArray *videoTracks = [asset tracksWithMediaType:AVMediaTypeVideo];
    if (!videoTracks) {
        return;
    }
    AVAssetTrack *track = videoTracks[0];
    // 获取视频的帧率信息
    CGFloat fps = track.nominalFrameRate;
    NSLog(@"formats %f",track.nominalFrameRate);
    
    /** AVAssetImageGenerator 用来从视频中提取图片的一个对象
     *  本地和远程文件都可以用该方法进行提取
     */
    AVAssetImageGenerator *imageGen = [[AVAssetImageGenerator alloc] initWithAsset:asset];
    // 定义请求指定时间上的误差范围，这里设置为0，代表尽量精确
    imageGen.requestedTimeToleranceAfter = kCMTimeZero;
    imageGen.requestedTimeToleranceBefore = kCMTimeZero;
    
    // 构建10秒处的所有视频帧然后转换成图像
    CMTime start = CMTimeMake(10000, 1000);
    if (CMTimeGetSeconds(start) > CMTimeGetSeconds(asset.duration)) {
        start = CMTimeMake((CMTimeGetSeconds(asset.duration) - 1) * 1000, 1000);
    }
    NSMutableArray *times = [NSMutableArray array];
    
    for (int i=0; i<fps; i++) {
        CMTime oneFps = CMTimeMake(i/fps * 1000, 1000);
        CMTime time = CMTimeAdd(start, oneFps);
        NSValue *timeValue = [NSValue valueWithCMTime:time];
        [times addObject:timeValue];
    }
    __block BOOL finish = NO;
    __block int num = 0;
    // 1、此方法用于请求多个视频帧，效率比较高，备注：返回的image由系统自动释放内存
    [imageGen generateCGImagesAsynchronouslyForTimes:times completionHandler:^(CMTime requestedTime, CGImageRef  _Nullable image, CMTime actualTime, AVAssetImageGeneratorResult result, NSError * _Nullable error) {
        NSLog(@"actual time %f",CMTimeGetSeconds(actualTime));
        num++;
        if (num == fps) {
            finish = YES;
        }
        
        @autoreleasepool {
            UIImage *imageObj = [[UIImage alloc] initWithCGImage:image];
            NSData *jpgData = UIImageJPEGRepresentation(imageObj, 1.0);
            NSString *spath = [path stringByAppendingFormat:@"/%d.JPG",num];
            [jpgData writeToFile:spath atomically:YES];
        }
    }];
    
    while (!finish) {
        usleep(1000);
    }
    NSLog(@"写入时间段结束了");
    
    // 2、提取指定时间的一个视频帧
    CMTime atime = CMTimeMake(5*1000, 1000);
    CMTime actime = kCMTimeZero;
    NSError *error = nil;
    CGImageRef cgimage = [imageGen copyCGImageAtTime:atime actualTime:&actime error:&error];
    NSLog(@"need time %f actual time %f",CMTimeGetSeconds(atime),CMTimeGetSeconds(actime));
    UIImage *uiimage = [[UIImage alloc] initWithCGImage:cgimage];
    NSData *jpgData = UIImageJPEGRepresentation(uiimage, 1.0);
    NSString *atPath = [path stringByAppendingPathComponent:@"at.JPG"];
    [jpgData writeToFile:atPath atomically:YES];
    CGImageRelease(cgimage);
    
    // 3、提取指定时间的一个视频帧，并且进行压缩后输出
    // 默认maximumSize为CGSizeZero代表不压缩按照原始大小输出
    imageGen.maximumSize = CGSizeMake(300, 150);
    cgimage = [imageGen copyCGImageAtTime:atime actualTime:&actime error:&error];
    NSLog(@"need time %f actual time %f",CMTimeGetSeconds(atime),CMTimeGetSeconds(actime));
    uiimage = [[UIImage alloc] initWithCGImage:cgimage];
    jpgData = UIImageJPEGRepresentation(uiimage, 1.0);
    atPath = [path stringByAppendingPathComponent:@"at_scale.JPG"];
    [jpgData writeToFile:atPath atomically:YES];
    CGImageRelease(cgimage);
    
}
@end
