package com.example.demo.Audio;

public class ADOpenSLES implements AudioPlayInterface{
    static {
        System.loadLibrary("adMedia");
    }

    private String mPath;
    private int mSample_rate;
    private int mCh_layout;
    private int mFormat;

    private Thread mThread;

    /** 播放手机中的音频PCM文件
     *  path:PCM文件路径
     *  sample_rate:采样率 取值8000-48000
     *  ch_layout:声道类型 0 单声道 1双声道
     *  format:采样格式 0 8bit位宽 1 16bit位宽 2 32bit位宽
     *  采样率 取值8000-48000 ch_layout
     */
    public ADOpenSLES(final String path, final int sample_rate, final int ch_layout, final int format) {
        mPath = path;
        mSample_rate = sample_rate;
        mCh_layout = ch_layout;
        mFormat = format;
    }


    @Override
    public void play() {
        if (mThread == null) {
            mThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    playAudio(mPath,mSample_rate,mCh_layout,mFormat);
                }
            });
            mThread.start();
        }
    }

    public void stop() {
        stopAudio();

        if (mThread != null) {
            try {
                mThread.join(); // 等待线程结束
            } catch (InterruptedException io) {

            }
            mThread = null;
        }
    }

    /** 播放手机中的音频PCM文件
     *  path:PCM文件路径
     *  sample_rate:采样率 取值8000-48000
     *  ch_layout:声道类型 0 单声道 1双声道
     *  format:采样格式 0 8bit位宽 1 16bit位宽 2 32bit位宽
     *  采样率 取值8000-48000 ch_layout
     */
    native void playAudio(String path,int sample_rate,int ch_layout,int format);
    native void stopAudio();
}
