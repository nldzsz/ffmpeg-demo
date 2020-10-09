//
//  AVCapturePriviewer.m
//  demo-ios
//
//  Created by apple on 2020/10/8.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVCapturePriviewer.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

@interface AVCapturePriviewer()<AVCaptureFileOutputRecordingDelegate,AVCapturePhotoCaptureDelegate>
{
    // 采集管理会话
    AVCaptureSession *captureSession;
    // 采集工作队列，由于采集，开始，更换摄像头等等需要一定的耗时，所以需要放在子线程做做这些事情
    dispatch_queue_t sessionQueue;
    AVCaptureDeviceInput *videoInput;
    AVCaptureDeviceInput *audioInput;
    AVCaptureMovieFileOutput *fileOutput;
    AVCapturePhotoOutput *stillOutput;
    
    UIButton *stillImage;
    UIButton *record;
    UIButton *change;
    
    NSURL *dstURL;
    
    
    UIBackgroundTaskIdentifier taskId;
}
@end

@implementation AVCapturePriviewer

- (void)startCaptureMovieDst:(NSURL*)moveURL
{
    taskId = UIBackgroundTaskInvalid;
    dstURL = moveURL;
    stillImage = [UIButton buttonWithType:UIButtonTypeSystem];
    stillImage.frame = CGRectMake(self.bounds.size.width - 80, 0, 80, 30);
    [stillImage setTitle:@"stillImage" forState:UIControlStateNormal];
    [stillImage addTarget:self action:@selector(onTapButton:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:stillImage];


    record = [UIButton buttonWithType:UIButtonTypeSystem];
    record.frame = CGRectMake(self.bounds.size.width - 80, 40, 80, 30);
    [record setTitle:@"record" forState:UIControlStateNormal];
    [record addTarget:self action:@selector(onTapButton:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:record];

    change = [UIButton buttonWithType:UIButtonTypeSystem];
    change.frame = CGRectMake(self.bounds.size.width - 80, 80, 80, 30);
    [change setTitle:@"change" forState:UIControlStateNormal];
    [change addTarget:self action:@selector(onTapButton:) forControlEvents:UIControlEventTouchUpInside];
    [self addSubview:change];
    
    /** 关于相机权限和麦克风权限的总结：
     *  1、要想使用相机和麦克风首先得在Info.plist文件中添加NSMicrophoneUsageDescription音频使用权限
     *  和NSCameraUsageDescription相机使用权限
     *  2、首次使用APP，当使用AVCaptureDeviceInput进行初始化的时候就会弹出使用相机权限的对话框，如果用户拒绝赋予权限
     *  或者先赋予了又在设置里面拒绝给权限，那么这个初始化方法将返回nil
     *  4、用户删掉APP后对应的权限等等也一并删除
     *  3、所以正确的做法应该是如下的代码，根据当前权限状态来作相应的初始化
     */
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
        case AVAuthorizationStatusAuthorized:
        {
            NSLog(@"有视频权限");
            [self requestMicroAuthorized];
        }
        break;
        case AVAuthorizationStatusNotDetermined:
        {
            NSLog(@"首次打开APP则需要请求相机权限");
            // 此方法为异步的，用户选择完毕后回调被执行。如果APP还没有请求过权限，那么调用此方法会弹出一个对话框提示用户给与
            // 权限。如果用户已经拒绝过或者给予了权限则调用该方法无效
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
                NSLog(@"相机的选择为：%d",granted);
                if (granted) {
                    [self requestMicroAuthorized];
                }
            }];
        }
            break;
        default:
        {
            NSLog(@"用户拒绝给予相机权限，这时候可以调用自定义权限请求对话框请求权限");
            // 此时调用此方法无效
//            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
//                if (!granted) {
//                    NSLog(@"用户拒绝了给予相机权限");
//                }
//
//            }];
        }
            break;
    }
    
    NSLog(@"结束");
}

- (void)requestMicroAuthorized
{
    switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio]) {
        case AVAuthorizationStatusAuthorized:
        {
            NSLog(@"有麦克风权限了");
            [self setupCaptureSession];
        }
            break;
        case AVAuthorizationStatusNotDetermined:
        {
            NSLog(@"首次打开APP，还未请求过麦克风权限");
            [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
                if (granted) {
                    [self setupCaptureSession];
                }
                NSLog(@"音频权限 %d",granted);
            }];
        }
        default:
        {
            NSLog(@"不具有麦克风权限");
        }
            break;
    }
}

- (void)setupCaptureSession
{
    // 创建工作队列
    sessionQueue = dispatch_queue_create("CaputureQueue", DISPATCH_QUEUE_SERIAL);
    
    // 创建会话
    captureSession = [[AVCaptureSession alloc] init];
    // 设置会话的sessionPreset,代表了采集视频的宽高
    captureSession.sessionPreset = AVCaptureSessionPreset640x480;
    AVCaptureVideoPreviewLayer *videoLayer = [[AVCaptureVideoPreviewLayer alloc] init];
    videoLayer.frame = self.layer.bounds;
    videoLayer.session = captureSession;
    [self.layer addSublayer:videoLayer];
    
    dispatch_async(sessionQueue, ^{
        
        AVCaptureDevice *videoDeivce = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:AVCaptureDevicePositionBack];
        // 1、添加视频采集
        // 首次使用APP，调用该方法就会弹出使用相机权限的对话框，如果用户拒绝赋予权限
        // 或者先赋予了又在设置里面拒绝给权限，那么这个初始化方法将返回nil
        self->videoInput = [[AVCaptureDeviceInput alloc] initWithDevice:videoDeivce error:nil];
        [self->captureSession addInput:self->videoInput];
        
        
        /** AVCaptureConnection
         *  AVCaptureSession实际上是通过该对象构建起AVCaptureInput和AVCaptureOutput之间的连接
         *  当调用addInput:和addOut:时就会自动构建一个AVCaptureConnection对象。
         *  AVCaptureVideoPreviewLayer内部也包含一个AVCaptureOutput对象，当调用videoLayer.session
         *  时会自动调用addOut:
         *
         *  它代表了一个流对象，通过它可以设置输出视频的方向，可以设置输出视频是否增稳等等属性
         */
        videoLayer.connection.videoOrientation = AVCaptureVideoOrientationPortrait;
    //    NSLog(@"connections %@",captureSession.connections);
        
        
        // 2、添加音频采集
        AVCaptureDevice *audioDeivce = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInMicrophone mediaType:AVMediaTypeAudio position:AVCaptureDevicePositionUnspecified];
        // 首次使用APP，调用该方法就会弹出使用相机权限的对话框，如果用户拒绝赋予权限
        // 或者先赋予了又在设置里面拒绝给权限，那么这个初始化方法将返回nil
        self->audioInput = [[AVCaptureDeviceInput alloc] initWithDevice:audioDeivce error:nil];
        [self->captureSession addInput:self->audioInput];
        
        
        /** AVCaptureMovieFileOutput
         *  用于将来自采集的音视频输出自动压缩然后保存到指定的文件中
         *  它的文件容器格式只能是MOV，编码方式颜色格式音频参数等等都采用默认值
         *
         *  AVCaptureAudioFileOutput也是一样的工作方式
         */
        // 3、添加将采集的音视频保存到文件中
        self->fileOutput = [[AVCaptureMovieFileOutput alloc] init];
        [self->captureSession addOutput:self->fileOutput];
        // AVCaptureConnection必须在addOutput:方法调用之后才会生成AVCaptureConnection
        AVCaptureConnection *videoConn = [self->fileOutput connectionWithMediaType:AVMediaTypeVideo];
        NSDictionary *videoSettings = @{
            AVVideoCodecKey:AVVideoCodecH264
        };
        // 设置视频编码方式；它一般会有个默认编码方式，不同设备可能不一样，比如IphoneX 就是HEVC
        [self->fileOutput setOutputSettings:videoSettings forConnection:videoConn];
        
        
        // 4、添加拍照输出对象
        // 备注：会话管理对象一次性可以添加多个输入输出对象
        AVCapturePhotoOutput *stillOut = [[AVCapturePhotoOutput alloc] init];
        [self->captureSession addOutput:stillOut];
        self->stillOutput = stillOut;
        
        // 采集会话
        [self->captureSession startRunning];
        
        
    });
}


- (void)onTapButton:(UIButton*)btn
{
    if (btn == change) {
        [self changeCamera];
    } else if(btn == record) {
        [self recordStartOrStop:!fileOutput.recording];
    } else if(btn == stillImage) {
        [self takeStillImage];
    }
}

// 更换摄像头
- (void)changeCamera
{
    dispatch_async(sessionQueue, ^{
        
        AVCaptureDevicePosition curPostion = [self->videoInput.device position];
        if (curPostion == AVCaptureDevicePositionBack) {
            curPostion = AVCaptureDevicePositionFront;
        } else {
            curPostion = AVCaptureDevicePositionBack;
        }
        
        // 更换摄像头，只需要将以前AVCaptureInput删除，重新添加新的AVCaputreDeviceInput即可
        // 这个过程只需要卸载 beginConfiguration和commitConfiguration中
        AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithDeviceType:AVCaptureDeviceTypeBuiltInWideAngleCamera mediaType:AVMediaTypeVideo position:curPostion];
        AVCaptureDeviceInput *newInput = [[AVCaptureDeviceInput alloc] initWithDevice:device error:nil];
        [self->captureSession beginConfiguration];
        
        // 先删掉的输入对象，然后在添加新的，如果新的添加失败，则将以前的还原
        [self->captureSession removeInput:self->videoInput];
        if ([self->captureSession canAddInput:newInput]) {
            [self->captureSession addInput:newInput];
            self->videoInput = newInput;
        } else {
            [self->captureSession addInput:self->videoInput];
        }
        
        [self->captureSession commitConfiguration];
    });
}

- (void)recordStartOrStop:(BOOL)start
{
    dispatch_async(sessionQueue, ^{
        if (start) {
            // 当应用即将处于后台或者处于后台时，didFinishRecordingToOutputFileAtURL是不会调用的，也就是有可能文件没有完整保存。
            // 所以这里需要给应用申请多执行一段时间大概为180秒 该方法可以在APP的启动后任何地方调用，它必须和endBackgroundTask
            // 成对调用，否则会崩溃
            if (self->taskId == UIBackgroundTaskInvalid) {
                self->taskId = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:nil];
            }
            
            // 将采集的音视频自动保存到文件中
            [self->fileOutput startRecordingToOutputFileURL:self->dstURL recordingDelegate:self];
        } else {
            // 调用停止保存到文件方法
            [self->fileOutput stopRecording];
            
            NSLog(@"结束保存到文件");
        }
    });
    
}

- (void)takeStillImage
{
    dispatch_async(sessionQueue, ^{
        /** AVCapturePhotoSettings定义输出照片的数据格式已经生成文件的格式，例如压缩方式(JPEG,PNG,RAW)，文件格式(JPG,png,DNG)等等其他拍照参数
         *  也支持输出未压缩的原始数据CVPixelbufferRef，具体可以参考AVCapturePhotoSettings设置
         */
        // 代表默认输出JPG文件
        AVCapturePhotoSettings *defaultSettings = [AVCapturePhotoSettings photoSettings];
        defaultSettings.autoStillImageStabilizationEnabled = YES;
        // 是否输出原始拍照分辨率，默认NO(代表输出照片的分辨率和视频分辨率一样大，例如前面sessionPreset为640x480的，那么生成照片也是这样大的)
        // 备注：下面这两个必须同时打开，否则崩溃
        defaultSettings.highResolutionPhotoEnabled = YES;
        self->stillOutput.highResolutionCaptureEnabled = YES;
        /** AVCapturePhotoOutpt 是对AVCaptureStillImageOutput的升级，它支持Live Photo capture, preview-sized image delivery, wide color, RAW, RAW+JPG and RAW+DNG formats.
         *  照片
         */
        // 按照指定的输出格式执行拍照指令，具体的执行情况通过代理回调
        
        [self->stillOutput capturePhotoWithSettings:defaultSettings delegate:self];
    });
    
}

- (void)captureOutput:(AVCaptureFileOutput *)output didStartRecordingToOutputFileAtURL:(NSURL *)fileURL fromConnections:(NSArray<AVCaptureConnection *> *)connections
{
    [record setTitle:@"stop" forState:UIControlStateNormal];
    NSLog(@"record start url %@ thread %@",fileURL,[NSThread currentThread]);
}

- (void)captureOutput:(AVCaptureFileOutput *)output didFinishRecordingToOutputFileAtURL:(NSURL *)outputFileURL fromConnections:(NSArray<AVCaptureConnection *> *)connections error:(NSError *)error
{
    NSLog(@"record finish url %@ thread %@",outputFileURL,[NSThread currentThread]);
    // 结束之后调用此方法
    [[UIApplication sharedApplication] endBackgroundTask:self->taskId];
    
    [self->record setTitle:@"record" forState:UIControlStateNormal];
}



- (void)captureOutput:(AVCapturePhotoOutput *)output willCapturePhotoForResolvedSettings:(nonnull AVCaptureResolvedPhotoSettings *)resolvedSettings
{
    NSLog(@"即将开始拍照 %@",resolvedSettings);
}

- (void)captureOutput:(AVCapturePhotoOutput *)output didCapturePhotoForResolvedSettings:(AVCaptureResolvedPhotoSettings *)resolvedSettings
{
    NSLog(@"拍照完成 %@",resolvedSettings);
}

// ios 11+的获取照片方式
#if __IPHONE_11_0
- (void)captureOutput:(AVCapturePhotoOutput *)output didFinishProcessingPhoto:(AVCapturePhoto *)photo error:(NSError *)error
API_AVAILABLE(ios(11.0)){
    NSLog(@"didFinishProcessingPhoto ");
    NSString *dstPath = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory,NSUserDomainMask,YES)[0] stringByAppendingPathComponent:@"1-test_capture.JPG"];
    NSURL *dstURL = [NSURL fileURLWithPath:dstPath];
    NSData *jpg = [photo fileDataRepresentation];
    [jpg writeToURL:dstURL atomically:YES];
}
#else
// ios 10的获取照片方式
- (void)captureOutput:(AVCapturePhotoOutput *)output didFinishProcessingPhotoSampleBuffer:(CMSampleBufferRef)photoSampleBuffer previewPhotoSampleBuffer:(CMSampleBufferRef)previewPhotoSampleBuffer resolvedSettings:(AVCaptureResolvedPhotoSettings *)resolvedSettings bracketSettings:(AVCaptureBracketedStillImageSettings *)bracketSettings error:(NSError *)error
{
    NSData *jpgegData = [AVCapturePhotoOutput JPEGPhotoDataRepresentationForJPEGSampleBuffer:photoSampleBuffer previewPhotoSampleBuffer:previewPhotoSampleBuffer];
    NSString *dstPath = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory,NSUserDomainMask,YES)[0] stringByAppendingPathComponent:@"1-test_capture.JPG"];
    NSURL *dstURL = [NSURL fileURLWithPath:dstPath];
    [jpgegData writeToURL:dstURL atomically:YES];
}
#endif


- (void)captureOutput:(AVCapturePhotoOutput *)output didFinishCaptureForResolvedSettings:(nonnull AVCaptureResolvedPhotoSettings *)resolvedSettings error:(nullable NSError *)error
{
    NSLog(@"拍照完成 didFinishCaptureForResolvedSettings");
}
@end
