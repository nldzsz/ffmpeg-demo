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
        
        dispatch_semaphore_signal(self->semaphore);
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
    // 启用调用视频输入对象的写入工作队列
    [videoInput requestMediaDataWhenReadyOnQueue:vwriteQueue usingBlock:^{
        while (videoInput.readyForMoreMediaData) {
            // 说明可以向封装器写入数据了
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
                }
            } else {
                NSLog(@"说明视频读取完毕");
                videoFinish = YES;
                [videoInput markAsFinished];
                
            }
        }
        
        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了1");
            [writer finishWritingWithCompletionHandler:^{
                
            }];
        }
    }];
    
    [audioInput requestMediaDataWhenReadyOnQueue:awriteQueue usingBlock:^{
        while (audioInput.readyForMoreMediaData) {
            
            // 说明可以向封装器写入数据了
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
                }
            } else {
                NSLog(@"说明音频读取完毕");
                audioFinish = YES;
                [audioInput markAsFinished];
            }
        }
        
        if (videoFinish && audioFinish) {
            NSLog(@"真正结束了2");
            [writer finishWritingWithCompletionHandler:^{
                
            }];
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
         *  用于将数据写入容器，可以写入压缩数据也可以写入未压缩数据，如果最后一个参数为nil则代表对数据不做压缩处理直接写入容器，不为nil则代表对对数据
         *  按照指定格式压缩后写入容器
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

@end
