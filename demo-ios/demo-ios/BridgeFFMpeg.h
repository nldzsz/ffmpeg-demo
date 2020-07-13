//
//  BridgeFFMpeg.h
//  demo-ios
//
//  Created by apple on 2020/7/13.
//  Copyright © 2020 apple. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface BridgeFFMpeg : NSObject
+(void)testFFmpeg;
+(void)doEncodecPcm2aac:(NSString*)src dst:(NSString*)dst;
+(void)doResample:(NSString*)src dst:(NSString*)dst;
+(void)doResampleAVFrame:(NSString*)src dst:(NSString*)dst;
+(void)doScale:(NSString*)src dst:(NSString*)dst;
+(void)doDemuxer:(NSString*)src;
+(void)doMuxerTwoFile:(NSString*)audio_src video_src:(NSString*)video_src dst:(NSString*) dst;
+(void)doReMuxer:(NSString*)src dst:(NSString*) dst;
+(void)doSoftDecode:(NSString*)src;
+(void)doSoftDecode2:(NSString*)src;
+(void)doSoftEncode:(NSString*)src dst:(NSString*) dst;
+(void)doHardDecode:(NSString*) src;
+(void)doHardEncode:(NSString*)src dst:(NSString*) dst;
+(void)doReMuxerWithStream:(NSString*)src dst:(NSString*) dst;
+(void)doExtensionTranscode:(NSString*)src dst:(NSString*) dst;
+(void)doTranscode:(NSString*)src dst:(NSString*) dst;
+(void)doEncodeMuxer:(NSString*)dst;
+(void)doCut:(NSString*)src dst:(NSString*)dst start:(NSString*)start du:(int)du;
+(void)MergeTwo:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst;
+(void)MergeFiles:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst;
+(void)addMusic:(NSString*)src1 src2:(NSString*)src2 dst:(NSString*)dst start:(NSString*)start;
+(void)doJpgGet:(NSString*)src dst:(NSString*)dst start:(NSString*)start getmore:(BOOL)getmore num:(int)num;
+(void)doJpgToVideo:(NSString*)src dst:(NSString*) dst;
+(void)doChangeAudioVolume:(NSString*)src dst:(NSString*) dst;
+(void)doChangeAudioVolume2:(NSString*)src dst:(NSString*) dst;
+(void)doVideoScale:(NSString*)src dst:(NSString*) dst;
@end

NS_ASSUME_NONNULL_END
