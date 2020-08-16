//
//  ViewController.m
//  demo-ios
//
//  Created by apple on 2020/7/7.
//  Copyright © 2020 apple. All rights reserved.
//

#import "ViewController.h"
#import "EBDropdownListView.h"
#include "BridgeFFMpeg.h"


@interface ViewController ()
{
    EBDropdownListView *_dropdownListView;
    BOOL                isProcessing;
}
@property (strong, nonatomic) UIButton *playBtn;
@property (strong, nonatomic) UILabel *statusLabel;
@end

@implementation ViewController
- (NSArray*)itemsForffmpeg
{
    NSArray *dic = @[
        @"pcm2aac 0",
        @"doResample 1",
        @"doResampleAVFrame 2",
        @"doScale 3",
        @"doDemuxer 4",
        @"doMuxerTwoFile 5",
        @"doReMuxer 6",
        @"doSoftDecode 7",
        @"doSoftDecode2 8",
        @"doSoftEncode 9",
        @"video_doHardDecode 10",
        @"video_doHardEncode 11",
        @"doReMuxerWithStream 12",
        @"doExtensionTranscode 13",
        @"doTranscode 14",
        @"doEncodeMuxer 15",
        @"doCut 16",
        @"MergeTwo 17",
        @"MergeFiles 18",
        @"addMusic 19",
        @"doJpgGet 20",
        @"doJpgToVideo 21",
        @"doChangeAudioVolume 22",
        @"doChangeAudioVolume2 23",
        @"doVideoScale 24",
        @"doAudioacrossfade 25",
        @"addSubtitleStream 26",
        @"addSubtitlesForVideo 27"
    ];
    return dic;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor whiteColor];
    isProcessing = NO;
    
    NSArray *items = [self itemsForffmpeg];
    NSMutableArray *itemViews = [NSMutableArray array];
    NSInteger num = 0;
    for (NSString *itemDes in items) {
        NSString *key = [NSString stringWithFormat:@"%ld",num];
        EBDropdownListItem *item = [[EBDropdownListItem alloc] initWithItem:key itemName:itemDes];
        [itemViews addObject:item];
        num++;
    }
    _dropdownListView = [[EBDropdownListView alloc] initWithDataSource:itemViews];
    _dropdownListView.selectedIndex = 0;
    _dropdownListView.frame = CGRectMake(20, 100, 350, 30);
    [_dropdownListView setViewBorder:0.5 borderColor:[UIColor grayColor] cornerRadius:2];
    [self.view addSubview:_dropdownListView];
    
    self.statusLabel = [[UILabel alloc]initWithFrame:CGRectMake(10, 250,300, 50)];
    self.statusLabel.text = @"";
    self.statusLabel.textColor = [UIColor whiteColor];
    self.statusLabel.backgroundColor = [UIColor blackColor];
    [self.view addSubview:self.statusLabel];
    
    self.playBtn = [UIButton buttonWithType:UIButtonTypeSystem];
    self.playBtn.frame = CGRectMake(150, 370,100, 50);
    [self.playBtn setTitle:@"开始" forState:UIControlStateNormal];
    [self.playBtn setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    [self.view addSubview:self.playBtn];
    [self.playBtn addTarget:self action:@selector(onTapPlayBtn:) forControlEvents:UIControlEventTouchUpInside];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    CGRect org = self.statusLabel.frame;
    CGRect frame = self.view.frame;
    org.size.width = frame.size.width - 20;
    self.statusLabel.frame = org;
}

-(void)processFinish
{
    dispatch_async(dispatch_get_main_queue(), ^{
        self.statusLabel.text=@"处理完毕...";
    });
}
- (void)onTapPlayBtn:(UIButton*)btn
{
    
    if (isProcessing) {
        self.statusLabel.text=@"正在处理中...";
        return;
    }
    isProcessing = true;
    self.statusLabel.text=@"正在处理中...";

    NSString *path = NSSearchPathForDirectoriesInDomains(NSCachesDirectory,NSUserDomainMask,YES)[0];
    
    NSInteger _selectIndex = _dropdownListView.selectedIndex;
    switch (_selectIndex) {
        case 0:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_441_f32le_2.pcm" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test_441_f32le_2.aac"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doEncodecPcm2aac:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 1:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_441_f32le_2.pcm" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test_240_s32le_2.pcm"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doResample:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 2:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_441_f32le_2.pcm" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test_441_s32le_2.pcm"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doResampleAVFrame:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 3:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_640x360_yuv420p.yuv" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test.yuv"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doScale:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 4:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test-mp3-1.mp3" ofType:nil];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doDemuxer:pcmpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 5:
        {
            NSString *apath = [[NSBundle mainBundle] pathForResource:@"test-mp3-1.mp3" ofType:nil];
            NSString *vpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_2.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test.MOV"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doMuxerTwoFile:apath video_src:vpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 6:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"abc-test.h264" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"test_1280x720_1.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doReMuxer:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 7:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doSoftDecode:pcmpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 8:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_441_f32le_2.aac" ofType:nil];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doSoftDecode2:pcmpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 9:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_640x360_yuv420p.yuv" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-abc-test.h264"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doSoftEncode:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 10:
        {
            // 硬解码以及硬编码不支持模拟器
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doHardDecode:pcmpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 11:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_640x360_yuv420p.yuv" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"3-test.h264"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doHardEncode:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 12:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"abc-test.h264" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"2-test_1280_720_1.MP4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doReMuxerWithStream:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 13:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"2-test_1280x720_3.mov"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doExtensionTranscode:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 14:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-test_1280x720_3.mov"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doTranscode:pcmpath dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 15:
        {
            NSString *pcmpath = [path stringByAppendingPathComponent:@"11-test.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doEncodeMuxer:pcmpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 16:
        {
            NSString *pcmpath = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-cut-test_1280x720_3.mp4"];
            NSString *start = @"00:00:15";
            int duration = 5;
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doCut:pcmpath dst:dstpath start:start du:duration];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 17:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_1.mp4" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_4.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-merge_1.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg MergeTwo:pcmpath1 src2:pcmpath2 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 18:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_1.mp4" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"2-merge_1.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg MergeFiles:pcmpath1 src2:pcmpath2 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 19:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_4.mp4" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_441_f32le_2.aac" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"11_add_music.mp4"];
            NSString *start = @"00:00:15";
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg addMusic:pcmpath1 src2:pcmpath2 dst:dstpath start:start];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 20:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-doJpg_get%3d.jpg"];//1-doJpg_get.jpg
            NSString *start = @"00:00:15";
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doJpgGet:pcmpath1 dst:dstpath start:start getmore:TRUE num:5];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 21:
        {
            NSString *pcmpath1 = [path stringByAppendingPathComponent:@"1-doJpg_get%3d.jpg"];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-dojpgToVideo.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doJpgToVideo:pcmpath1 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 22:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test-mp3-1.mp3" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-addaudiovolome-1.mp3"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doChangeAudioVolume:pcmpath1 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 23:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test-mp3-1.mp3" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-addaudiovolome-2.mp3"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doChangeAudioVolume2:pcmpath1 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 24:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.mp4" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-videoscale_1.mp4"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doVideoScale:pcmpath1 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 25:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_mp3_1.mp3" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_mp3_2.mp3" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-output.mp3"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg doAcrossfade:pcmpath1 src2:pcmpath2 dst:dstpath duration:10];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 26:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_4.mp4" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.srt" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"1-subtitles-out.mkv"];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg addSubtitleStream:pcmpath1 src2:pcmpath2 dst:dstpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        case 27:
        {
            NSString *pcmpath1 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_4.mp4" ofType:nil];
            NSString *pcmpath2 = [[NSBundle mainBundle] pathForResource:@"test_1280x720_3.srt" ofType:nil];
            NSString *dstpath = [path stringByAppendingPathComponent:@"2-addsubtitles-video.mp4"];
            NSString *confdpath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"fonts.conf"];
            NSString *fontpath = [[NSBundle mainBundle] resourcePath];
            NSDictionary *fontmaped = @{@"Myfont":@"latin"};
            [BridgeFFMpeg configConfpath:confdpath fontsPath:fontpath withFonts:fontmaped];
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                [BridgeFFMpeg addSubtitlesForVideo:pcmpath1 src2:pcmpath2 dst:dstpath confdpath:confdpath];
                self->isProcessing = false;
                [self processFinish];
            });
        }
        break;
        default:
            break;
    }
}
@end
