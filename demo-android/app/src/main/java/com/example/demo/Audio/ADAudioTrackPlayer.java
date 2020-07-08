package com.example.demo.Audio;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import com.example.demo.Common.ByteUtil;
import com.example.demo.Common.DDlog;

import java.io.IOException;
import java.io.InputStream;

public class ADAudioTrackPlayer implements AudioPlayInterface{
    /** AudioTrack
     * 1、AudioTrack 是比较底层的android api，它只负责播放原始的PCM裸数据
     * 2、
     * */

    //// ========= audiotrack 播放asserts目录下的音频文件 =========/////
    /** 遇到问题 1、当文件大小超过1M时，无法通过AssetManager获取InputStream
     *  解决方案 1、MP3，arm等格式文件不受此限制，将PCM文件后缀改为.amr即可
     * */
    /** 遇到问题2：通过getAssets().openFd().getFileDescriptor()获取到的FileDescriptor来构造FileInputStream对象，读取数据时不是从文件开头读取
     *  解决方案：getAssets().open()方式获取InputStream来读取文件中数据，是从文件头开始的。
     * */

    private Context mContext;
    private AudioTrack aTrack;
    // 播放器的最小缓冲区大小
    private int mMinBufferSize;
    // 是否停止
    private boolean isStop;
    // 播放线程
    private Thread mThread;
    // 从播放文件中读取数据的流
    private InputStream mInputStream;
    // 音频数据是否大端序
    private boolean isBigendian;
    // 音频采样格式
    private int mFormat;

    /** 播放assets目录下的原始音频文件
     * 1、一个AudioTrack实例相当于一个音频会话，要播放音频则需要启动一个会话
     * Context c:应用程序上下文
     * String path:assset文件夹下的原始音频文件名
     * int sampleRate,int format,int ch_layout:采样率，采样格式，声道类型
     * boolean isBig:音频数据存储是大端序还是小端序
     */
    public ADAudioTrackPlayer(Context c,String path,int sampleRate,int format,int ch_layout,boolean isBig) {
        mContext = c;
        mFormat = format;
        isBigendian = isBig;
        initAudioTrackByBuilder(ch_layout,sampleRate,format);
        initInputStream(path);
    }

    /** 初始化AudioTrack
     * ch_layout:声道类型
     * format:采样格式 表示一个采样点使用多少位表示
     * sampleRate:一般有20khz，44.1khz 48khz等
     * */
    private void initAudioTrack(int ch_layout,int sampleRate,int format) {
        // 最好使用此函数计算缓冲区大小，而非自己手动计算
        mMinBufferSize = AudioTrack.getMinBufferSize(sampleRate, ch_layout, format);

        /**
         * int streamType:表示了不同的音频播放策略，按下手机的音量键，可以看到有多个音量管理，比如可以单独禁止警告音但是可以开启
         * 乐播放声音，这就是不同的音频播放管理策略；以常量形式定义在AudioManager中，如下：
         *      STREAM_MUSIC:播放音频用这个就好
         *      STREAM_VOICE_CALL:电话声音
         *      STREAM_ALARM:警告音
         *      ......
         * int sampleRateInHz:音频采样率
         * int channelConfig:声道类型;CHANNEL_IN_XXX适用于录制音频，CHANNEL_OUT_XXX用于播放音频
         * int audioFormat:采样格式
         * int bufferSizeInBytes:音频会话的缓冲区大小。音频播放时，app将音频原始数据不停的输送给这个缓冲区，然后AudioTrack不停从这个缓冲区拿数据送给音频播放系统
         * 从而实现声音的播放
         * int mode:缓冲区数据的流动方式;如下：
         * MODE_STREAM:流式流动，只缓存部分
         * MODE_STATIC:一次性缓冲全部数据，适用于音频比较小的播放
         * 备注：对于录制音频，为了性能考虑，最好用CHANNEL_IN_MoNo单声道，而转变立体声的过程在声音的特效处理阶段来完成
         * */
        aTrack = new AudioTrack(
                AudioManager.STREAM_MUSIC,// 指定流的类型
                sampleRate,// 设置音频数据的採样率 32k，假设是44.1k就是44100
                ch_layout,// 设置输出声道为双声道立体声，而CHANNEL_OUT_MONO类型是单声道
                format,// 设置音频数据块是8位还是16位。这里设置为16位。
                mMinBufferSize,//缓冲区大小
                AudioTrack.MODE_STREAM // 设置模式类型，在这里设置为流类型，第二种MODE_STATIC貌似没有什么效果
        );
    }

    /** 初始化AudioTrack，官方推荐此方法
     * ch_layout:声道类型
     * format:采样格式 表示一个采样点使用多少位表示
     * sampleRate:一般有20khz，44.1khz 48khz等
     * */
    private void initAudioTrackByBuilder(int ch_layout,int sampleRate,int format) {
        mMinBufferSize = AudioTrack.getMinBufferSize(sampleRate, ch_layout, format);
        aTrack = new AudioTrack.Builder()
                // AudioAttributes用来设置音频类型，相当于上面的streamType，如下是播放音频的策略
                .setAudioAttributes(new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_MEDIA)
                        .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)// 这两项一般都是对应设置
                        .build())
                .setAudioFormat(new AudioFormat.Builder()
                        .setEncoding(format)        // 采样格式
                        .setSampleRate(sampleRate)  // 采样率
                        .setChannelMask(ch_layout)  // 就是声道类型
                        .build())
                .setBufferSizeInBytes(mMinBufferSize)
                .build();
    }

    private void initInputStream(String path) {
        try {
            mInputStream = mContext.getAssets().open(path);
        } catch (IOException io) {

        }
    }

    // 将AudioTrack切换到播放状态
    public void play() {
        isStop = false;
        if (aTrack != null && aTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
            aTrack.play();
            startAudioThread();
        }
    }

    public void stop() {
        isStop = true;
        if (aTrack != null && aTrack.getState() != AudioTrack.STATE_UNINITIALIZED) {
            aTrack.stop();
        }

        // 停止线程
        if (mThread != null) {
            try {
                mThread.join();
                mThread = null;
            } catch (InterruptedException ie) {

            }
        }
        // 释放AudioTrack
        aTrack.release();
        aTrack = null;
    }

    // 开启播放线程
    private void startAudioThread() {
        if (mThread == null) {
            mThread = new Thread(new AudioThread());
            mThread.start();
        }
    }

    /** 遇到问题 1、对于输入数据为大端序无法正常播放
     *  解决方案 1、AudioTrack只能播放小端序的音频数据，所以对于大端序的数据得先转换成小端序在播放
     *
     *  遇到问题2、输入数据格式为float类型无法正常播放
     *  解决方案：由于inputStream读取的是字节，而当播放float类型数据时，write()函数写入的必须是float数组，所以写入之前要将byte[]数据转换成float[]数据
     * */
    private class AudioThread implements Runnable {
        @Override
        public void run() {
            DDlog.logd("AudioThread start mMinBufferSize==> "+mMinBufferSize);
            // 一次写入的数据可以是1024，不一定非得mMinBufferSize个字节
//            samples = new short[mMinBufferSize];
            while (!isStop) {
                try {
                    byte[] buffer = new byte[1024];
                    int sampleSize = mInputStream.read(buffer);
//                    DDlog.logd(ByteUtil.byte2hex(buffer));

                    // 向缓冲区写入数据，此函数为阻塞行数，一般写入200ms数据需要接近200ms时间
                    if (mFormat == AudioFormat.ENCODING_PCM_FLOAT) {
                        float[] samples = ByteUtil.bytesToFloats(buffer, sampleSize, isBigendian);
                        aTrack.write(samples,0,samples.length,AudioTrack.WRITE_BLOCKING);
                    } else if (mFormat == AudioFormat.ENCODING_PCM_16BIT) {
                        short[] samples = ByteUtil.bytesToShorts(buffer, sampleSize, isBigendian);
                        aTrack.write(samples, 0, samples.length);
                    } else {
                        aTrack.write(buffer, 0, sampleSize);
                    }

                } catch (IOException io) {

                }
            }
        }
    }
}
