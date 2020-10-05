//
//  AVDemuxer.m
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVDemuxer.h"
@interface AVDemuxer()<AVAssetDownloadDelegate>
{
    dispatch_semaphore_t decodeSemaphore;
}
@property(nonatomic,strong)NSURL *url;
@property(nonatomic,strong)NSURL *remoteurl;
@property(nonatomic,strong)AVAssetDownloadTask *downTask;

@end

@implementation AVDemuxer

+ (void)testGeneric
{
    // 代表了AVFoundation支持的媒体格式类型的全称
    NSLog(@"types %@",[AVURLAsset audiovisualTypes]);
    // 代表了AVFoundation支持的媒体格式类型的简称
    NSLog(@"types %@",[AVURLAsset audiovisualMIMETypes]);
    // 判断是否支持MOV格式;AVFoundation默认是支持MOV、MP4等容器格式的
    NSLog(@"yes %d",[AVURLAsset isPlayableExtendedMIMEType:@"video/quicktime"]);
}
- (id)initWithURL:(NSURL*)localURL
{
    if (!(self = [super init])) {
        return  nil;
    }
    
    self.url = localURL;
    self.autoDecode = YES;
    
    /** 遇到问题：AVDemuxer对象释放后程序崩溃
     *  分析原因：当对象释放时GCD会检查信号量的值，如果其值小于等于信号量初始化时的值 会认为其处于in use状态，所以会对其dispose 时就会崩溃
     *  解决方案：用如下代码替换之前dispatch_semaphore_create(1);的写法
     */
    decodeSemaphore = dispatch_semaphore_create(0);
    dispatch_semaphore_signal(decodeSemaphore);
    return self;
}

- (void)startProcess
{
    if (dispatch_semaphore_wait(decodeSemaphore, DISPATCH_TIME_NOW) != 0) {
        return;
    }
    
    NSDictionary *inputOptions = @{
        AVURLAssetPreferPreciseDurationAndTimingKey:@(YES)
    };
    /** AVAsset对象
     * 1、它是一个抽象类，是AVFoundation中对封装格式资源的封装，比如MP4容器，MKV容器，MP3容器等等。AVURLAsset对象则是它的一个具体子类
     * 通常通过该子类来初始化一个AVAsset对象。容器的属性(如音视频时长，视频帧率，编解码格式等等)通过键值对的方式存储于AVAsset对象中；
     * 2、初始化AVURLAsset对象的资源可以是本地的MP4资源，也可以是远程的基于HTTP协议的MP4资源；访问AVURLAsset对象的属性有两种方式，同步和异步
     *    a、直接调用属性名如下,inputAsset.tracks,inputAsset.duration等等是同步的方式，它的机制为：如果属性值未初始化，那么将阻塞当前线程去解封装容器的资源去
     * 初始化属性，当为远程资源时该过程会比较耗时。
     *    b、异步方式，通过loadValuesAsynchronouslyForKeys方法将对应的属性名传递进去异步获取，比如@[@"tracks",@"duration"]将异步获取这两个属性
     *
     *  后续的从容器中读取数据读取以及写入数据到容器中都需要依赖此对象
     */
    // 1、初始化AVAsset对象
    AVURLAsset *inputAsset = [[AVURLAsset alloc] initWithURL:self.url options:inputOptions];
//    self.asset = [[AVURLAsset alloc] initWithURL:[NSURL URLWithString:@"https://images.flypie.net/test_1280x720_3.mp4"] options:inputOptions];
    // 如果在这里直接调用如下属性，那么将采用同步方式初始化属性，会阻塞当前线程，一般本地资源是可以采用如下方式
//    NSLog(@"duration %f",CMTimeGetSeconds(inputAsset.duration));
//    NSLog(@"tracks %@",inputAsset.tracks);
    __block AVDemuxer *blockSelf = self;
    // 2、解析容器格式，作用和ffmpeg的avformat_open_input()函数功能一样。初始化AVAsset对象通过此方式异步初始化属性;
    // 备注：inputAsset不能被释放，否则初始化会失败，回调函数不在主线程中
    CFAbsoluteTime startTime = CFAbsoluteTimeGetCurrent();
    [inputAsset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
//        NSLog(@"thread %@",[NSThread currentThread]);
        
        NSError *error = nil;
        AVKeyValueStatus status = [inputAsset statusOfValueForKey:@"tracks" error:&error];
        if (status != AVKeyValueStatusLoaded) {
            NSLog(@"error %@",error);
            return;
        }
        
        NSLog(@"aync tracks %@",inputAsset.tracks);
        // 3、创建音视频读数据读取对象
        blockSelf.asset = inputAsset;
        [blockSelf processAsset];
        
        NSLog(@"总耗时 %f秒",CFAbsoluteTimeGetCurrent() - startTime);
        
        // 任务完毕
        dispatch_semaphore_signal(self->decodeSemaphore);
        NSLog(@"结束111");
        
        blockSelf = nil;
    }];
    
    // 阻塞当前线程
    dispatch_semaphore_wait(decodeSemaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束");
}

- (void)processAsset
{
    self.assetReader = [self createAssetReader];
    if (!self.assetReader) {
        return;
    }
    
    AVAssetReaderOutput *videoTrackout = nil;
    AVAssetReaderOutput *audioTrackout = nil;
    BOOL videoFinish = NO;
    BOOL audioFinish = YES;
    for (AVAssetReaderOutput *output in self.assetReader.outputs) {
        if ([output.mediaType isEqualToString:AVMediaTypeVideo]) {
            videoTrackout = output;
        }
        if ([output.mediaType isEqualToString:AVMediaTypeAudio]) {
            audioTrackout = output;
            audioFinish = NO;
        }
    }
    
    // 开始读取;不会阻塞
    if ([self.assetReader startReading] == NO) {
        NSLog(@"start reading failer");
        return;
    }
    
    // 通过读取状态来判断是否还有未读取完的音视频数据
    CMSampleBufferRef videoSamplebuffer = NULL;
    CMSampleBufferRef audioSamplebuffer = NULL;
    
    int sum = 0;
    while (self.assetReader.status == AVAssetReaderStatusReading && (!videoFinish || !audioFinish)) {
        
        [self readNextVideoFrameFromOutput:videoTrackout];

        if ( (audioTrackout) && (!audioFinish) )
        {
                [self readNextAudioSampleFromOutput:audioTrackout];
        }
    }
    
    /** 遇到问题：
     */
    if (self.assetReader.status == AVAssetReaderStatusCompleted) {
        [self.assetReader cancelReading];
        self.assetReader = nil;
    }
}

- (BOOL)readNextVideoFrameFromOutput:(AVAssetReaderOutput *)readerVideoTrackOutput;
{
    if (self.assetReader.status == AVAssetReaderStatusReading)
    {
        // 从视频输出对象读取视频数据，如果没有了，则返回NULL
        CMSampleBufferRef sampleBufferRef = [readerVideoTrackOutput copyNextSampleBuffer];
        if (sampleBufferRef)
        {
            NSLog(@"read a video frame: %@", CFBridgingRelease(CMTimeCopyDescription(kCFAllocatorDefault, CMSampleBufferGetOutputPresentationTimeStamp(sampleBufferRef))));

            
            CMSampleBufferInvalidate(sampleBufferRef);
            CFRelease(sampleBufferRef);

            return YES;
        }
    }

    return NO;
}

- (BOOL)readNextAudioSampleFromOutput:(AVAssetReaderOutput *)readerAudioTrackOutput;
{
    if (self.assetReader.status == AVAssetReaderStatusReading)
    {
        CMSampleBufferRef audioSampleBufferRef = [readerAudioTrackOutput copyNextSampleBuffer];
        if (audioSampleBufferRef)
        {
            NSLog(@"read an audio frame: %@", CFBridgingRelease(CMTimeCopyDescription(kCFAllocatorDefault, CMSampleBufferGetOutputPresentationTimeStamp(audioSampleBufferRef))));
//            [self.audioEncodingTarget processAudioBuffer:audioSampleBufferRef];
            CFRelease(audioSampleBufferRef);
            return YES;
        }
    }
    
    return NO;
}

- (AVAssetReader *)createAssetReader
{
    NSError *error = nil;
    /** AVAssetReader对象
     *  它跟AVCaptureSession作用一样，作为从容器对象AVAsset对象读取数据的管理器，需要音视频输出对象
     *
     *  备注：此种方式只支持本地容器的数据读取
     */
    AVAssetReader *assetReader = [AVAssetReader assetReaderWithAsset:self.asset error:&error];
    if (error) {
        NSLog(@"error %@",error);
        return  nil;
    }
    NSMutableDictionary *videoOutputSettings = [NSMutableDictionary dictionary];
    if ([AVDemuxer supportsFastTextureUpload]) {
        [videoOutputSettings setObject:@(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange) forKey:(__bridge NSString*)kCVPixelBufferPixelFormatTypeKey];
    } else {
        [videoOutputSettings setObject:@(kCVPixelFormatType_32BGRA) forKey:(__bridge NSString*)kCVPixelBufferPixelFormatTypeKey];
    }
    
    /** AVAssetTrack 对象
     *  1、音视频流对象，它代表了容器中的某一路音视频流，跟ffmpeg中AVStream对象功能一样。
     *  2、一个容器中可以包括一路视频流或者多路音频流或者多路字幕流
     *  3、当AVAsset对象被初始化之后，音视频流对象就被初始化了，通过如下方式获取该对象
     */
    AVAssetTrack *videoTrack = [[self.asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
    /** AVAssetReaderTrackOutput 对象
     *  1、音视频输出对象，它是AVAssetReaderOutput对象的具体实现子类，外部通过该对象读取音视频数据
     *  2、该对象负责配置输出音视频数据的格式等等参数
     *  3、音视频输出对象要加入到音视频读取管理对象中才能从其中读取数据
     */
    // 添加视频输出对象;备注：当最后一个参数为nil时代表输出压缩的数据；
    // 如果配置了outputSettings 则内部会自动调用gpu硬解码输出解压后的数据，解压后的数据格式为前面outputSettings中配置的；
    if (!self.autoDecode) {
        videoOutputSettings = nil;
    }
    AVAssetReaderTrackOutput *videoTrackOut = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:videoTrack outputSettings:videoOutputSettings];
    videoTrackOut.alwaysCopiesSampleData = NO;
    [assetReader addOutput:videoTrackOut];
    
    // 添加音频输出对象
    AVAssetTrack *audioTrack = [[self.asset tracksWithMediaType:AVMediaTypeAudio] objectAtIndex:0];
    AVAssetReaderTrackOutput *audioTrackOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:audioTrack outputSettings:nil];
    audioTrackOutput.alwaysCopiesSampleData = NO;
    [assetReader addOutput:audioTrackOutput];
    
    return  assetReader;
}

- (id)initWithRemoteURL:(NSURL*)remoteURL
{
    if (self = [super init]) {
        self.remoteurl = remoteURL;
        self.autoDecode = NO;
        
        decodeSemaphore = dispatch_semaphore_create(0);
        dispatch_semaphore_signal(decodeSemaphore);
    }
    
    return self;
}

- (void)startRemoteProcess
{
    if (dispatch_semaphore_wait(decodeSemaphore, DISPATCH_TIME_NOW) != 0) {
        return;
    }
    [self processRemoteAssset];
    
//    // 初始化AVURLAsset容器对象
//    NSDictionary *options = @{
//        AVURLAssetPreferPreciseDurationAndTimingKey:@(YES)
//    };
//    self.asset = [AVURLAsset URLAssetWithURL:self.remoteurl options:options];
//    // 异步初始化容器属性
//    __weak typeof(self) weakSelf = self;
//    [self.asset loadValuesAsynchronouslyForKeys:@[@"tracks",@"duration"] completionHandler:^{
//
//        AVKeyValueStatus status = [weakSelf.asset statusOfValueForKey:@"tracks" error:nil];
//        if (status != AVKeyValueStatusLoaded) {
//            NSLog(@"load property failed");
//            return;
//        }
//
//        NSLog(@"tracks %@",weakSelf.asset.tracks);
//        NSLog(@"duration %f",CMTimeGetSeconds(weakSelf.asset.duration));
//
//        [weakSelf processRemoteAssset];
//
//        dispatch_semaphore_signal(self->decodeSemaphore);
//
//    }];
    
    dispatch_semaphore_wait(decodeSemaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束111");
}

- (void)processRemoteAssset
{
    NSURLSessionConfiguration *config = [NSURLSessionConfiguration backgroundSessionConfigurationWithIdentifier:@"title"];
    config.URLCredentialStorage = nil;
    
    AVAssetDownloadURLSession *session = [AVAssetDownloadURLSession sessionWithConfiguration:config assetDownloadDelegate:self delegateQueue:[NSOperationQueue mainQueue]];
    NSDictionary *avoptions = @{AVURLAssetPreferPreciseDurationAndTimingKey:@(YES)};
    // https://images.flypie.net/test_1280x720_3.mp4
    NSString *urlString = @"https://bitdash-a.akamaihd.net/content/sintel/hls/playlist.m3u8";
    AVURLAsset *url = [AVURLAsset URLAssetWithURL:[NSURL URLWithString:urlString] options:avoptions];
    self.downTask = [session assetDownloadTaskWithURLAsset:url assetTitle:@"title" assetArtworkData:nil options:nil];
    
    // 开始下载
    [self.downTask resume];
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error
{
    NSLog(@"error %@",error);
}

- (void)URLSession:(NSURLSession *)session assetDownloadTask:(AVAssetDownloadTask *)assetDownloadTask didResolveMediaSelection:(AVMediaSelection *)resolvedMediaSelection
{
    NSLog(@"开始111 %@",resolvedMediaSelection);
}

- (void)URLSession:(NSURLSession *)session assetDownloadTask:(AVAssetDownloadTask *)assetDownloadTask didFinishDownloadingToURL:(NSURL *)location
{
    NSLog(@"开始111 location %@",location);
}

- (void)URLSession:(NSURLSession *)session assetDownloadTask:(AVAssetDownloadTask *)assetDownloadTask didLoadTimeRange:(CMTimeRange)timeRange totalTimeRangesLoaded:(NSArray<NSValue *> *)loadedTimeRanges timeRangeExpectedToLoad:(CMTimeRange)timeRangeExpectedToLoad
{
    NSLog(@"开始111 %f %f %@",CMTimeGetSeconds(timeRange.duration),CMTimeGetSeconds(timeRangeExpectedToLoad.duration),loadedTimeRanges);
}

- (void)stopProcess
{
    
}

+ (BOOL)supportsFastTextureUpload
{
#if TARGET_IPHONE_SIMULATOR
    return  NO;
#endif
    
    // IOS5 之后就支持了
    return YES;
}
@end
