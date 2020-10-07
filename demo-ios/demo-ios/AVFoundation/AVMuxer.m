//
//  AVMuxer.m
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVMuxer.h"
#import <AVFoundation/AVFoundation.h>

@implementation AVMuxer
{
    dispatch_semaphore_t semaphore;
    dispatch_queue_t    awriteQueue;
    dispatch_queue_t    vwriteQueue;
}
- (void)remuxer:(NSURL*)srcURL dstURL:(NSURL*)dstURL
{
    semaphore = dispatch_semaphore_create(0);
    awriteQueue = dispatch_queue_create("awriteQueue", DISPATCH_QUEUE_SERIAL);
    vwriteQueue = dispatch_queue_create("vwriteQueue", DISPATCH_QUEUE_SERIAL);
    
    // 创建AVURLAsset容器对象
    AVURLAsset *urlAsset = [[AVURLAsset alloc] initWithURL:srcURL options:nil];
    // 异步初始化容器对象中的属性
    [urlAsset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
        AVAssetReaderTrackOutput *videoOutput = nil,*audioOutput = nil;
        AVAssetReader *reader = [self createAssetReader:urlAsset videoOutput:&videoOutput audioOutput:&audioOutput];
        [self doDemuxer:reader videoOutput:videoOutput audioOutput:audioOutput thenMuxerTo:dstURL];
    }];
    
    // 等待任务完成
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束了");
}

- (AVAssetReader*)createAssetReader:(AVAsset*)asset videoOutput:(AVAssetReaderTrackOutput**)videoOutput audioOutput:(AVAssetReaderTrackOutput**)audioOutput
{
    // 创建音视频输出管理对象，通过该对象开启和结束对外界输出音视频
    NSError *error = nil;
    AVAssetReader *reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
    if (error) {
        NSLog(@"create AVAssetReader failer");
        return nil;
    }
    
    BOOL foundVideoTrack = NO,foundAudioTrack = NO;
    NSArray *tracks = [asset tracks];
    for (int i=0; i<tracks.count; i++) {
        AVAssetTrack *track = tracks[i];
        if ([track.mediaType isEqualToString:AVMediaTypeVideo] && !foundVideoTrack) {
            // 视频轨道对象
            NSLog(@"视频轨道");
            
            // 通过视频轨道对象创建视频输出对象，第二个参数为nil代表从容器读取视频数据输出时保留原格式，不解码
            AVAssetReaderTrackOutput *videoTrackOut = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:nil];
            // 将视频输出对象添加到音视频输出管理器对象中
            if (![reader canAddOutput:videoTrackOut]) {
                NSLog(@"can add videooutput");
                return nil;
            }
            [reader addOutput:videoTrackOut];
            *videoOutput = videoTrackOut;
            foundVideoTrack = YES;
        } else if ([track.mediaType isEqualToString:AVMediaTypeAudio] && !foundAudioTrack) {
            // 从AVAsset对象获取音频轨道对象
            NSLog(@"音频轨道");
            // 通过AVAssetTrack对象创建音频输出对象;第二个参数为nil代表输出的音频不经过解码保留原格式
            AVAssetReaderTrackOutput *audioTrackOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:track outputSettings:nil];
            // 将音频输出对象添加到音视频输出管理器对象中
            [reader addOutput:audioTrackOutput];
            *audioOutput = audioTrackOutput;
            foundAudioTrack = YES;
        }
    }
    
    return reader;
}

- (void)doDemuxer:(AVAssetReader*)reader videoOutput:(AVAssetReaderTrackOutput*)videoOutput audioOutput:(AVAssetReaderTrackOutput*)audioOutput thenMuxerTo:(NSURL*)dstURL
{
    // 开启从容器中读取数据然后输出音视频数据
    if (![reader startReading]) {
        NSLog(@"reader failer");
        return;
    }
    
    // 创建封装器
    AVAssetWriterInput *videoInput = nil,*audioInput = nil;
    AVAssetWriter *writer = [self createAssetWriter:dstURL videoTrack:videoOutput.track audioTrack:audioOutput.track];
    
    // 从封装器中编译出输入对象
    __block BOOL audioFinish = YES;
    __block BOOL videoFinish = YES;
    __block BOOL audioWriteFinish = YES;
    __block BOOL videoWriteFinish = YES;
    for (AVAssetWriterInput *input in writer.inputs) {
        if ([input.mediaType isEqualToString:AVMediaTypeAudio]) {
            audioInput = input;
            audioFinish = NO;
            audioWriteFinish = NO;
        }
        
        if ([input.mediaType isEqualToString:AVMediaTypeVideo]) {
            videoInput = input;
            videoFinish = NO;
            videoWriteFinish = NO;
        }
    }
    
    // 封装器开始写入输入标记
    if (![writer startWriting]) {
        NSLog(@"writer failer");
        return;
    }
    
    __block BOOL firstSample = YES;
    /** 音视频写入监控队列，该队列的工作原理
     *  1、此方法调用后不会阻塞，当输入对象处于可写状态并且可以向输入对象写入数据时，block会回调，该回调block会一直周期性的回调
     *  直到输入队列处于不可写状态，所以就可以再此回调调用appendSampleBuffer写入数据了，这样能保证正常写入
     *  2、videoInput不会持有该回调block
     *  3、如果不通过requestMediaDataWhenReadyOnQueue回调方式而是直接调用appendSampleBuffer写入数据，则有可能会出错，因为音视频
     *  两个输入对象是共用的一个写入管理器，可能处于不可写状态
     *
     */
    [videoInput requestMediaDataWhenReadyOnQueue:vwriteQueue usingBlock:^{
        
        // 虽然block会周期性的回调，不过一般在这里通过循环来持续性的写入数据，readyForMoreMediaData为YES代表现在处于可写状态了
        while (videoInput.readyForMoreMediaData) {

            // 如果容器中音频和视频全部被读取完毕，那么status才会从AVAssetReaderStatusReading变成AVAssetReaderStatusCompleted
            // 所以可以通过这个status来监听容器中数据是否全部读取完毕
            // 备注：音视频中只有一个被读取完毕status的值仍为AVAssetReaderStatusReading
            if (reader.status == AVAssetReaderStatusReading) {
                CMSampleBufferRef samplebuffer = [videoOutput copyNextSampleBuffer];
                
                if (firstSample && samplebuffer) {
                    
                    firstSample = NO;
                    [writer startSessionAtSourceTime:CMSampleBufferGetOutputPresentationTimeStamp(samplebuffer)];
                }
                
                if (samplebuffer) {
                    [self printSamplebuffer:samplebuffer video:YES];
                    BOOL result = [videoInput appendSampleBuffer:samplebuffer];
                    NSLog(@"video writer %d",result);
                } else {
                    // 容器中音视频数据时长可能有些差距，那么会出现视频或者音频先读取完毕的情况，如果要监控音频或者视频中的某一个是否读取完毕
                    // 那么通过判断samplebuffer是否为NULL
                    videoFinish = YES;
                    // 当调用markAsFinished之后，readyForMoreMediaData会变为NO，这个回调方法也不会再调用了
                    // 该方法可以写在任何地方，不影响
                    [videoInput markAsFinished];
                    NSLog(@"没数据啦");
                }
            }
//            else {
//                NSLog(@"状态 %ld",reader.status);
//            }
        }

        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了1");
            // 当音视频输入对象都调用markAsFnish函数后，就需要调用此方法结束整个封装，此方法可以写在这个回调里面也可以
            // 写在任何地方
            [writer finishWritingWithCompletionHandler:^{
                
            }];
            dispatch_semaphore_signal(self->semaphore);
        }
    }];
    
    [audioInput requestMediaDataWhenReadyOnQueue:awriteQueue usingBlock:^{
        
        while (audioInput.readyForMoreMediaData) {  // 说明可以向封装器写入数据了
            
            if (reader.status == AVAssetReaderStatusReading) {
                CMSampleBufferRef samplebuffer = [audioOutput copyNextSampleBuffer];
                
                if (firstSample && samplebuffer) {
                    firstSample = NO;
                    [writer startSessionAtSourceTime:CMSampleBufferGetOutputPresentationTimeStamp(samplebuffer)];
                }
                
                if (samplebuffer) {
                    [self printSamplebuffer:samplebuffer video:NO];
                    BOOL result = [audioInput appendSampleBuffer:samplebuffer];
                    NSLog(@"video writer %d",result);
                } else {
                    NSLog(@"说明音频读取完毕");
                    audioFinish = YES;
//                    [audioInput markAsFinished];
                }
            }
//            else {
//                NSLog(@"我的状态 %ld",reader.status);
//            }
        }
        
        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了2");
            [writer finishWritingWithCompletionHandler:^{
                
            }];
            dispatch_semaphore_signal(self->semaphore);
        }
    }];
}

- (void)printSamplebuffer:(CMSampleBufferRef)samplebuffer video:(BOOL)video
{
    static int vnum = 0,anum = 0;
    CGFloat pts = CMTimeGetSeconds(CMSampleBufferGetOutputPresentationTimeStamp(samplebuffer));
    CGFloat dts = CMTimeGetSeconds(CMSampleBufferGetOutputDecodeTimeStamp(samplebuffer));
    CGFloat dur = CMTimeGetSeconds(CMSampleBufferGetOutputDuration(samplebuffer));
    size_t size = CMSampleBufferGetTotalSampleSize(samplebuffer);
    
    if (video) {
        vnum++;
        /** CMFormatDescriptionRef对象(格式描述对象)
         *  1、CMVideoFormatDescriptionRef、CMAudioFormatDescriptionRef、CMTextFormatDescriptionRef是它的具体子类，分别
         *  对应着视频、音频、字幕的封装参数对象
         *  2、所有关于音视频等等的编解码参数，宽高等等都存储在此对象中
         *
         *  对于视频来说，它包括编码参数，宽高以及extension扩展参数，CMFormatDescriptionGetExtensions可以查看扩展参数内容
         *
         *  对于一个容器中读取出来的所有音/视频数据对象CMSampleBufferRef，音频对应着一个CMFormatDescriptionRef，视频
         *  对应着一个CMFormatDescriptionRef(即所有视频数据对象得到的格式描述对象地址都一样)，音频也是一样
         */
//            NSLog(@"extension %@",CMFormatDescriptionGetExtensions(curformat));
//        NSLog(@"CMFormatDescriptionRef %@",CMSampleBufferGetFormatDescription(samplebuffer));
        NSLog(@"video pts(%f) dts(%f) duration(%f) size(%ld) num(%d)",pts,dts,dur,size,vnum);
    } else {
        anum++;
        NSLog(@"audio pts(%f) dts(%f) duration(%f) size(%ld) num(%d)",pts,dts,dur,size,anum);
    }
}

- (AVAssetWriter *)createAssetWriter:(NSURL*)dstURL videoTrack:(AVAssetTrack*)vtrack audioTrack:(AVAssetTrack*)atrack
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
    AVAssetWriter *writer = [AVAssetWriter assetWriterWithURL:dstURL fileType:AVFileTypeMPEG4 error:&error];
    if (error) {
        NSLog(@"create writer failer");
        return nil;
    }
    
    // 往封装器中添加音视频输入对象，每添加一个输入对象代表要往容器中添加一路流，一般添加一路视频流
    if (vtrack) {
        /** AVAssetWriterInput 对象
         *  用于将数据写入容器，可以写入压缩数据也可以写入未压缩数据，如果outputSettings为nil则代表对数据不做压缩处理直接写入容器，
         *  不为nil则代表对对数据按照指定格式压缩后写入容器
         *  最后一个参数sourceFormatHint为CMFormatDescriptionRef类型，表示封装相关的参数信息，当outputSettings为nil时，该参数必须设定
         *  否则无法封装(MOV格式除外)
         */
        CMFormatDescriptionRef srcformat = (__bridge CMFormatDescriptionRef)(vtrack.formatDescriptions[0]);
        AVAssetWriterInput *videoInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:nil sourceFormatHint:srcformat];
        // 将写入对象添加到封装器中
        [writer addInput:videoInput];
    }
    
    if (atrack) {
        CMFormatDescriptionRef srcformat = (__bridge CMFormatDescriptionRef)(atrack.formatDescriptions[0]);
        AVAssetWriterInput *audioInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:nil sourceFormatHint:srcformat];
        [writer addInput:audioInput];
    }
 
    return writer;
}


- (void)transcodec:(NSURL *)srcURL dstURL:(NSURL *)dstURL
{
    semaphore = dispatch_semaphore_create(0);
    awriteQueue = dispatch_queue_create("awriteQueue.com", DISPATCH_QUEUE_SERIAL);
    vwriteQueue = dispatch_queue_create("vwriteQueue.com", DISPATCH_QUEUE_SERIAL);
    
    AVURLAsset *inputAsset = [AVURLAsset assetWithURL:srcURL];
    [inputAsset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
        AVAssetReader *reader = [self createReader:inputAsset];
        [self demuxer:reader dstUrl:dstURL];
    }];
    
    dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    NSLog(@"结束了");
}

- (AVAssetReader*)createReader:(AVAsset*)asset
{
    NSError *error = nil;
    AVAssetReader *reader = [AVAssetReader assetReaderWithAsset:asset error:&error];
    if (error) {
        NSLog(@"create reader failer");
        return nil;
    }
    
    // 创建视频输出对象
    AVAssetTrack *videoTrack = [[asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0];
    // 由于这里要从h264编码变成h265编码，所以就需要视频的格式为YUV的
    /** 遇到问题：编码视频提示Domain=AVFoundationErrorDomain Code=-11800 "The operation could not be completed"
     *  分析原因：iOS不支持kCVPixelFormatType_422YpCbCr8BiPlanarFullRange，这里写错了应该是kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
     *  解决方案：改成kCVPixelFormatType_420YpCbCr8BiPlanarFullRange
     */
    NSDictionary *videoOutputs = @{
        (id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_420YpCbCr8BiPlanarFullRange)
    };
    AVAssetReaderTrackOutput *readerVideoTrackOut = [[AVAssetReaderTrackOutput alloc] initWithTrack:videoTrack outputSettings:videoOutputs];
    readerVideoTrackOut.alwaysCopiesSampleData = NO;
    [reader addOutput:readerVideoTrackOut];
    
    // 创建音频输出对象
    AVAssetTrack *audioTrack = [[asset tracksWithMediaType:AVMediaTypeAudio] objectAtIndex:0];
    NSDictionary *audioSettings = @{
        AVFormatIDKey:@(kAudioFormatLinearPCM),
    };
    AVAssetReaderTrackOutput *readerAudioOutput = [[AVAssetReaderTrackOutput alloc] initWithTrack:audioTrack outputSettings:audioSettings];
    readerAudioOutput.alwaysCopiesSampleData = NO;
    [reader addOutput:readerAudioOutput];
    
    return reader;
}

- (void)demuxer:(AVAssetReader*)reader dstUrl:(NSURL*)dstUrl
{
    AVAsset *asset = reader.asset;
    
    // 获取视频相关参数信息
    CMFormatDescriptionRef srcVideoformat = (__bridge CMFormatDescriptionRef)[[asset tracksWithMediaType:AVMediaTypeVideo] objectAtIndex:0].formatDescriptions[0];
    CMVideoDimensions demensions = CMVideoFormatDescriptionGetDimensions(srcVideoformat);
    
    // 创建封装器，通过封装器的回调函数驱动来获取数据
    NSError *error = nil;
    // zsz:todo 又一次忘记写了这句代码
    unlink([dstUrl.path UTF8String]);
    AVAssetWriter *writer = [AVAssetWriter assetWriterWithURL:dstUrl fileType:AVFileTypeQuickTimeMovie error:&error];
    if (error) {
        NSLog(@"create writer failer");
        dispatch_semaphore_signal(semaphore);
        return;
    }
    
    // 添加音视频输入对象
    // 对于音频来说 编码方式，采样率，采样格式，声道数，声道类型等等是必须的参数，比特率可以不用设置
    // 对于食品来说 编码方式，视频宽和高则是必须参数
    // 低端机型不支持H265编码
    NSDictionary *videoSettings = @{
        AVVideoCodecKey:AVVideoCodecHEVC,
        AVVideoWidthKey:@(demensions.width),
        AVVideoHeightKey:@(demensions.height)
    };
    AVAssetWriterInput *videoInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:videoSettings];
    videoInput.expectsMediaDataInRealTime = NO;
    [writer addInput:videoInput];
    
    // 获取音频相关参数
    CMFormatDescriptionRef audioFormat = (__bridge CMFormatDescriptionRef)[[asset tracksWithMediaType:AVMediaTypeAudio] objectAtIndex:0].formatDescriptions[0];
    // 备注，这个extensions并不是音频相关参数，不知道是什么。
//    CFDictionaryRef properties = CMFormatDescriptionGetExtensions(audioFormat);
    // 获取音频相关参数
    const AudioStreamBasicDescription *audioDes = CMAudioFormatDescriptionGetStreamBasicDescription(audioFormat);
    size_t layout_size = 0;
    // 对于音频来说 编码方式，采样率，采样格式，声道数，声道类型等等是必须的参数，比特率可以不用设置
    // 对于食品来说 编码方式，视频宽和高则是必须参数
    const AudioChannelLayout *layout = CMAudioFormatDescriptionGetChannelLayout(audioFormat, &layout_size);
    // ios 不支持mp3的编码
    NSDictionary *audioSettings = @{
        AVFormatIDKey:@(kAudioFormatMPEG4AAC),
        AVSampleRateKey:@(audioDes->mSampleRate),
        AVChannelLayoutKey:[NSData dataWithBytes:layout length:layout_size],
        AVNumberOfChannelsKey:@(audioDes->mChannelsPerFrame),
    };
    AVAssetWriterInput *audioInput = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:audioSettings];
    /** 遇到问题：设置音频输入对象的expectsMediaDataInRealTime为YES，提示Domain=AVFoundationErrorDomain Code=-11800 "
     *  The operation could not be completed"
     *  分析原因：暂时没太明白，音视频采集的时候这个属性也都设置为YES了也没有问题，这里出问题
     *  解决方案：将此属性设置为NO
     */
    audioInput.expectsMediaDataInRealTime = NO;
    [writer addInput:audioInput];
    
    //开启解封装和封装
    if (![reader startReading]) {
        NSLog(@"reader startReading failer %@",reader.error);
        dispatch_semaphore_signal(semaphore);
        return;
    }
    if (![writer startWriting]) {
        NSLog(@"writer startWriting failer %@",writer.error);
        dispatch_semaphore_signal(semaphore);
        return;
    }
    
    AVAssetReaderOutput *videoOutput = nil,*audioOutput = nil;
    for (AVAssetReaderOutput *output in reader.outputs) {
        if ([output.mediaType isEqualToString:AVMediaTypeVideo]) {
            videoOutput = output;
        } else {
            audioOutput = output;
        }
    }
    
    __block BOOL firstWrite = YES;
    __block BOOL videoFinish = NO;
    __block BOOL audioFinish = NO;
    // 配置写入音视频数据的工作队列
    /** 遇到问题：模拟器发现内存消耗几个G，实际设备发现内存消耗正常
     *  分析原因：模拟器用cpu指令模拟GPU，内存模拟GPU的内存,因为这里编解码都是用的gpu,所以模拟器看起来内存消耗非常高
     *  解决方案：正常现象
     */
    [videoInput requestMediaDataWhenReadyOnQueue:vwriteQueue usingBlock:^{
        while (videoInput.readyForMoreMediaData) {  // 说明可以开始写入视频数据了

            if (reader.status == AVAssetReaderStatusReading) {
                CMSampleBufferRef samplebuffer = [videoOutput copyNextSampleBuffer];

                if (samplebuffer) {
                    // 从视频输出对象中读取数据
                    CMTime pts = CMSampleBufferGetOutputPresentationTimeStamp(samplebuffer);

                    if (firstWrite) {
                        firstWrite = NO;

                        [writer startSessionAtSourceTime:pts];
                    }

                    // 向视频输入对象写入数据
                    BOOL result = [videoInput appendSampleBuffer:samplebuffer];

                    NSLog(@"video writer %d",result);
                    if (!result) {
                        NSLog(@"video writer error %@",writer.error);
                    }
                    CMSampleBufferInvalidate(samplebuffer);
                    CFRelease(samplebuffer);
                } else {
                   NSLog(@"说明视频数据读取完毕");
                   videoFinish = YES;

                   // 源文件中视频数据读取完毕，那么就不需要继续写入视频数据了，将视频输入对象标记为结束
                   [videoInput markAsFinished];
               }
            }
        }

        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了1");
            [writer finishWritingWithCompletionHandler:^{
                [reader cancelReading];
                dispatch_semaphore_signal(self->semaphore);
            }];
        }
    }];
    
    // 配置写入音频数据的工作队列
    [audioInput requestMediaDataWhenReadyOnQueue:awriteQueue usingBlock:^{
        while (audioInput.readyForMoreMediaData) {  // 说明可以开始写入数据了

            if (reader.status == AVAssetReaderStatusReading) {
                CMSampleBufferRef samplebuffer = [audioOutput copyNextSampleBuffer];
                if (samplebuffer) {

                    // 从输出对象中读取数据
                    CMTime pts = CMSampleBufferGetOutputPresentationTimeStamp(samplebuffer);

                    if (firstWrite) {
                        firstWrite = NO;

                        [writer startSessionAtSourceTime:pts];
                    }

                    // 向视频输入对象写入数据
                    BOOL result = [audioInput appendSampleBuffer:samplebuffer];
                    NSLog(@"audio writer %d",result);
                    if (!result) {
                        NSLog(@"audio writer error %@",writer.error);
                    }
                    CMSampleBufferInvalidate(samplebuffer);
                    CFRelease(samplebuffer);
                } else {
                    NSLog(@"说明音频数据读取完毕1111");
                    audioFinish = YES;

                    // 源文件中视频数据读取完毕，那么就不需要继续写入视频数据了，将视频输入对象标记为结束
                    [audioInput markAsFinished];
                }
            }
        }

        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了2");
            [writer finishWritingWithCompletionHandler:^{
                [reader cancelReading];
                dispatch_semaphore_signal(self->semaphore);
            }];

        }
    }];
}
@end
