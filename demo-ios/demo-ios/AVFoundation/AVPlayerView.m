//
//  AVPlayerView.m
//  demo-ios
//
//  Created by apple on 2020/10/9.
//  Copyright © 2020 apple. All rights reserved.
//

#import "AVPlayerView.h"
#import <AVFoundation/AVFoundation.h>

static void *KVOcontenxt = &KVOcontenxt;

@interface AVPlayerView()
{
    AVURLAsset *_asset;
    
    AVPlayer *_player;
    AVPlayerItem *_playerItem;
    AVPlayerLayer *_playerLayer;
}
@end
@implementation AVPlayerView

// 要想使用AVPlayer播放视频，则layerClass必须返回AVPlayerLayer
+(Class)layerClass
{
    return [AVPlayerLayer class];
}

- (void)startPlayer:(NSURL*)vieoURL
{
    _playerLayer = (AVPlayerLayer*)self.layer;
    
    // AVURLAsset 作为容器对象，是进行播放的前提条件
//    _asset = [AVURLAsset assetWithURL:vieoURL];
    NSURL *newUrl = [NSURL URLWithString:@"https://images.flypie.net/test_1280x720_3.mp4"];
    _asset = [AVURLAsset assetWithURL:newUrl];

    // 异步解析文件的属性到容器中，远程文件的属性解析可能耗时比较久，所以这里采用异步方式加载，此方法的block不管加载成功与失败，最后都会回调
    // 如果是超时失败也会回调，只是这个超时时间暂时还不清楚
    __weak typeof(self) weakSelf = self;
    [_asset loadValuesAsynchronouslyForKeys:@[@"tracks"] completionHandler:^{
        NSError *error = nil;
        AVKeyValueStatus status = [self->_asset statusOfValueForKey:@"tracks" error:&error];
        if (status != AVKeyValueStatusLoaded) {
            NSLog(@"load failer %@",error);
            return;
        }
        
        if (!self->_asset.playable || self->_asset.hasProtectedContent) {
            NSLog(@"can not play || has protectedContent");
            return;
        }
        
        [weakSelf processAsset:self->_asset];
    }];
}

- (void)processAsset:(AVAsset*)asset
{
    /** AVPlayerItem
     *  代表了一个播放对象，它应该是AVAssetReader和AVAssetWriter的封装，它内部自动解析AVAsset的资源
     *  同时向外部提供音视频数据，专门用于向AVPlayer提供音视频数据
     *
     *  它和AVPlayer结合能播放远程的音视频文件，同时它对HLS协议也是完美支持。而AVAssetReader是不能解封装
     *  远程资源文件的
     */
    _playerItem = [[AVPlayerItem alloc] initWithAsset:asset];
    
    /** AVPlayer对象
     *  可以用来播放音视频的一个对象，它以AVPlayerItem作为输入(为其提供音视频数据)，
     *  再其内部实现了音频的渲染，视频通过AVPlayerLayer来渲染。如果只是播放音频那么
     *  就不需要AVPlayerLayer了
     */
    _player = [[AVPlayer alloc] initWithPlayerItem:_playerItem];
    
    /** AVPlayerLayer和AVPlayer结合在一起用来渲染视频用的
     *  可以设置视频的宽高比例，等等属性
     */
    _playerLayer.player = _player;
    
    
    // AVPLayerItem和AVPlayer的属性都是KVO的，所以可以通过KVO监控播放的相关的属性
    // 由于AVPlayer相关属性是异步加载的，如下这样输出资源的时长是没有值的，所以这里通过KVO来异步监
    // 控这些属性的变化。其它属性类似
    NSLog(@"duration %f",CMTimeGetSeconds(_player.currentItem.duration));
    [self addObserver:self forKeyPath:@"_player.currentItem.duration" options:NSKeyValueObservingOptionNew context:KVOcontenxt];
    
    // 开始播放
    [_player play];
    
}

- (void)dealloc
{
    // KVO要记得及时清除
    [self removeObserver:self forKeyPath:@"_player.currentItem.duration" context:KVOcontenxt];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
    if (context != KVOcontenxt) {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
        return;
    }
    
    if ([keyPath isEqualToString:@"_player.currentItem.duration"]) {
        NSValue *newvalue = change[NSKeyValueChangeNewKey];
        CMTime time = newvalue.CMTimeValue;
        NSLog(@"duration %f",CMTimeGetSeconds(time));
    }
    
}
@end
