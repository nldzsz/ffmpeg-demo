package com.example.demo.Audio;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;

import com.example.demo.Common.DDlog;
import com.example.demo.Common.PathTool;

import java.io.IOException;

public class ADAudioMediaPlayer implements AudioPlayInterface{

    // 上下文
    private Context mContext;
    // 播放方式 0 通过rawId,1 通过path 2 通过 Uri
    private int mType = 0;

    /** MediaPlayer类
     * 1、它位于安卓的多媒体框架android.media包下
     * 2、它可以直接播放音视频文件，既可以播放本地音视频，也可以播放网络音视频
     * 本例实现播放音频
     * */
    // 音频播放器
    private MediaPlayer madioPlayer;

    // 直接用Raw资源文件播放
    public ADAudioMediaPlayer(Context ct,int rawId) {
        mContext = ct;

        // 初始化
        madioPlayer = MediaPlayer.create(mContext,rawId);
        mType = 0;
    }

    // 播放本地文件
    public ADAudioMediaPlayer(Context ct,String path) {
        mContext = ct;
        mType = 1;

        madioPlayer = new MediaPlayer();
        /**
         *  通过这种方式，只需要创建一次MediaPlayer对象即可，当需要更换播放文件时，不用重复创建对象；这对于播放列表文件或重复播放使用
         * */
        try {
            AssetFileDescriptor as = PathTool.getAssetFileDescriptor(mContext,path);
//            madioPlayer.setDataSource(as.getFileDescriptor());    // 此方式播放会出错
            madioPlayer.setDataSource(as);
            madioPlayer.prepare();

        } catch (IOException io) {
            io.printStackTrace();
        }
    }

    /** 播放远程音频文件
     * 注意要添加访问网络的权限 <uses-permission android:name="android.permission.INTERNET" />
     * */
    public ADAudioMediaPlayer(Context ct,Uri uri) {
        mContext = ct;
        DDlog.logd(uri.toString());
        madioPlayer = new MediaPlayer();
        mType = 2;

        try {
            madioPlayer.setDataSource(mContext,uri);
            madioPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
        } catch (IOException io) {
            io.printStackTrace();
        }
    }

    @Override
    public void play() {
        // 直接播放，不需要调用preapare()函数
        if (mType == 0 || mType == 1) {
            madioPlayer.start();
        } else if (mType == 2) {
            madioPlayer.prepareAsync();
            madioPlayer.setOnPreparedListener(new MediaPlayer.OnPreparedListener() {
                @Override
                public void onPrepared(MediaPlayer mp) {
                    madioPlayer.start();
                }
            });
        }
    }

    // 播放完成后 要释放资源
    public void stop() {
        if (madioPlayer != null) {
            madioPlayer.stop();
            madioPlayer.release();
        }
    }
}
