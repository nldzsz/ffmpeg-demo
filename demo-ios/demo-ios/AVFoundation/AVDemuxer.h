//
//  AVDemuxer.h
//  demo-ios
//
//  Created by apple on 2020/10/3.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@interface AVDemuxer : NSObject
@property(nonatomic,strong)AVAsset *asset;
@property(nonatomic,strong)AVAssetReader *assetReader;
@property(nonatomic,assign)BOOL autoDecode;

- (id)initWithURL:(NSURL*)localURL;

/** 实现解封装MP4文件，并且将其中的未压缩音视频数据读取出来
 */
- (void)startProcess;
- (void)stopProcess;
@end
