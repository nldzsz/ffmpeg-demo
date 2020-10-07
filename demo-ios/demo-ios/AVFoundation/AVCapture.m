//
//  AVCapture.m
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVCapture.h"


/** 要开启音视频采集，必须在项目配置文件Info.plist中添加NSMicrophoneUsageDescription音频使用权限
 *  和NSCameraUsageDescription相机使用权限
 */
@interface AVCapture()<AVCaptureVideoDataOutputSampleBufferDelegate,AVCaptureAudioDataOutputSampleBufferDelegate>
{
    // 采集相关对象
    AVCaptureSession *session;
    AVCaptureVideoDataOutput *videoOutput;
    AVCaptureAudioDataOutput *audioOutput;
    
    
    // 封装相关对象
    AVAssetWriter *writer;
    AVAssetWriterInput *writerAudioInput;
    AVAssetWriterInput *WriterVideoInput;
    
    dispatch_queue_t acaptureQueue;
    dispatch_queue_t vcaptureQueue;
    dispatch_semaphore_t samaphore;
    
    BOOL firstsample;
    BOOL isWrite;
    AVFileType filetype;
}
@end
@implementation AVCapture
- (void)startCaptureToURL:(NSURL*)dstURL duration:(float)time fileType:(AVFileType)type
{
    acaptureQueue = dispatch_queue_create("acaptureQueue.com", DISPATCH_QUEUE_SERIAL);
    vcaptureQueue = dispatch_queue_create("vcaptureQueue.com", DISPATCH_QUEUE_SERIAL);
    samaphore = dispatch_semaphore_create(0);
    firstsample = YES;
    isWrite = YES;
    filetype = type;
    
    // 创建采集会话
    if (![self createCaptureSession]) {
        return;
    }
    
    // 开启音视频封装器的工作
    [self startAssetWriter:dstURL];
    
    //  备注，此方法会阻塞当前线程，所以最好放在子线程中调用，不要阻塞组现场
    [session startRunning];
    
    // time 秒后停止录制
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, time*NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        [self->writerAudioInput markAsFinished];
        [self->WriterVideoInput markAsFinished];
        [self->writer finishWritingWithCompletionHandler:^{
                    
        }];
        self->isWrite = NO;
    });
    
    dispatch_semaphore_wait(samaphore, DISPATCH_TIME_FOREVER);
    
}

- (BOOL)createCaptureSession
{
    /** AVCaptureSession 采集会话对象，它一头连接着输入对象(比如麦克风采集音频，摄像头采集视频)
     *  一头连接着输出对象向app提供采集好的原始音视频数据
     *  通过它管理采集的开始与结束
     */
    session = [[AVCaptureSession alloc] init];
    // 设置视频采集的宽高参数
    [session setSessionPreset:AVCaptureSessionPreset1280x720];
    
    // 实际的音频采集物理设备对象，通过此对象来创建音频输入对象；备注postion一定要是AVCaptureDevicePositionUnspecified
    // 否则会返回nil
    AVCaptureDevice *micro = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInMicrophone mediaType:AVMediaTypeAudio position:AVCaptureDevicePositionUnspecified];
    // 通过音频物理设备对象来创建音频输入对象,如果前面micro为nil这里也不会崩溃，audioInput会返回nil
    AVCaptureDeviceInput *audioInput = [[AVCaptureDeviceInput alloc] initWithDevice:micro error:nil];
    if (![session canAddInput:audioInput]) {    // audioInput为nil这里也不会崩溃，会放回NO
        NSLog(@"can not add audioInput");
        return NO;
    }
    [session addInput:audioInput];
    
    // 实际的视频采集物理设备对象,这里选择后置摄像头
    AVCaptureDevice *camera = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionBack];
    // 通过视频物理设备对象创建视频输入对象
    AVCaptureDeviceInput *videoInput = [[AVCaptureDeviceInput alloc] initWithDevice:camera error:nil];
    if (![session canAddInput:videoInput]) {
        NSLog(@"can not add video input");
        return NO;
    }
    [session addInput:videoInput];
    
    // 如果调用了[session startRunning]之后要想改变音视频输出对象配置参数，则必须调用[session beginConfiguration];和
    // [session commitConfiguration];才能生效。如果没有调用[session startRunning]则这两句代码可以不写
    [session beginConfiguration];
    // AVCaptureVideoDataOutput 创建视频输出对象，对象用于向外部输出视频数据，通过该对象设置向外部输入的视频数据格式
    // 比如像素格式(iOS只支持kCVPixelFormatType_32BGRA/kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
    // kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange三种格式)
    videoOutput = [[AVCaptureVideoDataOutput alloc] init];
    NSDictionary *videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
    };
    [videoOutput setVideoSettings:videoSettings];
    // 当采集速度过快而处理速度跟不上时的丢弃策略，默认丢弃最新采集的视频。这里设置为NO，表示不丢弃缓存起来
    videoOutput.alwaysDiscardsLateVideoFrames = NO;
    [videoOutput setSampleBufferDelegate:self queue:vcaptureQueue];
    [session addOutput:videoOutput];
    
    // AVCaptureAudioDataOutput 创建音频输出对象
    audioOutput = [[AVCaptureAudioDataOutput alloc] init];
    [audioOutput setSampleBufferDelegate:self queue:acaptureQueue];
    [session addOutput:audioOutput];
    
    
    [session commitConfiguration];
    
    return YES;
}

- (void)startAssetWriter:(NSURL*)dstURL
{
    NSError *error = nil;
    // AVAssetWriter 音视频写入对象管理器，通过该对象来控制写入的开始和结束以及管理音视频输入对象
    /** AVAssetWriter对象
     *  1、封装器对象，用于将音视频数据(压缩或未压缩数据)写入文件
     *  2、可以单独写入音频或视频，如果写入未压缩的音视频数据，AVAssetWriter内部会自动调用编码器进行编码
     */
    /** 遇到问题：调用startWriting提示Error Domain=AVFoundationErrorDomain Code=-11823 "Cannot Save"
     *  分析原因：如果封装器对应的文件已经存在，调用此方法时会提示这样的错误
     *  解决方案：调用此方法之前先删除已经存在的文件
     */
    unlink([dstURL.path UTF8String]);
    writer = [AVAssetWriter assetWriterWithURL:dstURL fileType:AVFileTypeMPEG4 error:&error];
    if (error) {
        NSLog(@"create writer failer");
        return;
    }
    
    // 往封装器中添加音视频输入对象，每添加一个输入对象代表要往容器中添加一路流，一般添加一路视频流
    /** AVAssetWriterInput 对象
     *  用于将数据写入容器，可以写入压缩数据也可以写入未压缩数据，如果outputSettings为nil则代表对数据不做压缩处理直接写入容器，
     *  不为nil则代表对对数据按照指定格式压缩后写入容器
     *
     */
    // 方式一、手动创建参数，不推荐。设置不对容易出错，其它参数采用默认值
//    NSDictionary *videoSettings = @{
//        AVVideoCodecKey:AVVideoCodecH264,
//        AVVideoWidthKey:@(640),
//        AVVideoHeightKey:@(480)
//    };
    // 方式二、由苹果系统返回
    NSDictionary *videoSettings = [videoOutput recommendedVideoSettingsForAssetWriterWithOutputFileType:filetype];
    WriterVideoInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
    WriterVideoInput.expectsMediaDataInRealTime = YES;
    // 将写入对象添加到封装器中
    [writer addInput:WriterVideoInput];
    
    /** 遇到问题：调用appendSampleBuffer写入音视频数据时出错
     *  分析原因：刚开始设置的AVEncoderBitRateKey码率为6400，太小，压缩可能要比较长时间，导致时间比较久就出错了
     *  解决方案：将码率设置为合适的值96000
     */
    // 方式一、手动创建参数，不推荐。设置不对容易出错,其它参数采用默认值
    // 编码方式，采样格式，采样率
    AudioChannelLayout layout;
    bzero(&layout, sizeof(layout));
    layout.mChannelLayoutTag = kAudioChannelLayoutTag_Mono;
    NSDictionary *audioSettings = @{
        AVFormatIDKey:@(kAudioFormatMPEG4AAC),
        AVChannelLayoutKey:[NSData dataWithBytes:&layout length:sizeof(layout)],
        AVNumberOfChannelsKey:@(1),
        AVSampleRateKey:@(44100),
        AVEncoderBitRateKey:@(96000)
    };
    // 方式二、由苹果系统返回
//    NSDictionary *audioSettings = [audioOutput recommendedAudioSettingsForAssetWriterWithOutputFileType:filetype];
    writerAudioInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:audioSettings];
    writerAudioInput.expectsMediaDataInRealTime = YES;
    [writer addInput:writerAudioInput];
    
    BOOL reuslt = [writer startWriting];
    if (!reuslt) {
        NSLog(@"start writer %@",writer.error);
    }
}

- (void)processSample:(CMSampleBufferRef)samplebuffer
{
    CMFormatDescriptionRef sampleFormat = CMSampleBufferGetFormatDescription(samplebuffer);
    CMMediaType mediatype = CMFormatDescriptionGetMediaType(sampleFormat);
    CMTime pts = CMSampleBufferGetPresentationTimeStamp(samplebuffer);
    
    if (mediatype == kCMMediaType_Video) {    // 视频
        if (firstsample) {
            firstsample = NO;
            [writer startSessionAtSourceTime:pts];
        }
        
        if (WriterVideoInput.readyForMoreMediaData) {
            BOOL reulst = [WriterVideoInput appendSampleBuffer:samplebuffer];
            NSLog(@"writer video %d",reulst);
        }
        
        
    } else if(mediatype == kCMMediaType_Audio) {    // 音频
        if (firstsample) {
            firstsample = NO;
            [writer startSessionAtSourceTime:pts];
        }
        
        if (writerAudioInput.readyForMoreMediaData) {
            BOOL reulst = [writerAudioInput appendSampleBuffer:samplebuffer];
            NSLog(@"writer audio %d",reulst);
        }
        
    } else {
        NSLog(@"其他数据");
    }
}

- (void)captureOutput:(AVCaptureOutput *)output didDropSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    NSLog(@"drop sample");
}

/** 前面虽然定义了音视频采集的工作队列(且是一个串行队列)，但是这个代理的线程是不固定的，随机的
 */
- (void)captureOutput:(AVCaptureOutput *)output didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    if (!isWrite) {
        NSLog(@"结束录制...");
        dispatch_semaphore_signal(samaphore);
        return;
    }
    
    if (output == audioOutput) {
        // 获取音频数据的大小
        size_t size = CMSampleBufferGetTotalSampleSize(sampleBuffer);
        NSLog(@"audio thread %@ size %ld",[NSThread currentThread],size);
    } else if(output == videoOutput){
        // 获取视频数据的大小，视频数据大小不能通过上面CMSampleBufferGetTotalSampleSize获取
        CVImageBufferRef imagebuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
        size_t size = CVPixelBufferGetDataSize(imagebuffer);
        NSLog(@"video thread %@ size %ld",[NSThread currentThread],size);
    } else {
        NSLog(@"unknown output");
    }
    
    [self processSample:sampleBuffer];
}
@end
