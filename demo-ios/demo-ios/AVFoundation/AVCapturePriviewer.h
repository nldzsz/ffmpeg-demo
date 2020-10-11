//
//  AVCapturePriviewer.h
//  demo-ios
//
//  Created by apple on 2020/10/8.
//  Copyright © 2020 apple. All rights reserved.
//

#import <UIKit/UIKit.h>


@interface AVCapturePriviewer : UIView

/** 实现AVFoundation采集的视频实时预览，同时能够拍摄高分辨率的照片
 *  将采集到的音视频保存到MOV中
 *
 *  备注：通过AVCaptureMovieFileOutput只能保存为MOV文件格式
 */
- (void)startCaptureMovieDst:(NSURL*)moveURL;
@end
