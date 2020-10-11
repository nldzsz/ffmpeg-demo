//
//  AVPlayerView.h
//  demo-ios
//
//  Created by apple on 2020/10/9.
//  Copyright © 2020 apple. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface AVPlayerView : UIView

/** 实现一个简单的音视频播放器，只有播放功能。
 *  至于快进，快退，播放暂停，滑动播放很好实现。
 */
- (void)startPlayer:(NSURL*)vieoURL;
@end
