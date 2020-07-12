package com.example.demo.FFmpegTest;

public class FFmpegTest {
    static {
        System.loadLibrary("adMedia");
    }


    public native static void testFFmpeg();
    public native static void doEncodecPcm2aac(String src,String dst);
    public native static void doResample(String src,String dst);
    public native static void doResampleAVFrame(String src,String dst);
    public native static void doScale(String src,String dst);
    public native static void doDemuxer(String src);
    public native static void doMuxerTwoFile(String audio_src,String video_src,String dst);
    public native static void doReMuxer(String src,String dst);
    public native static void doSoftDecode(String src);
    public native static void doSoftDecode2(String src);
    public native static void doSoftEncode(String src,String dst);
    public native static void doHardDecode(String src);
    public native static void doHardEncode(String src,String dst);
    public native static void doReMuxerWithStream(String src,String dst);
    public native static void doExtensionTranscode(String src,String dst);
    public native static void doTranscode(String src,String dst);
    public native static void doEncodeMuxer(String dst);
    public native static void doCut(String src,String dst,String start,int du);
    public native static void MergeTwo(String src1,String src2,String dst);
    public native static void MergeFiles(String src1,String src2,String dst);
    public native static void addMusic(String src1,String src2,String dst,String start);
    public native static void doJpgGet(String src,String dst,String start,boolean getmore,int num);
    public native static void doJpgToVideo(String src,String dst);
    public native static void doChangeAudioVolume(String src,String dst);
    public native static void doChangeAudioVolume2(String src,String dst);
    public native static void doVideoScale(String src,String dst);
}
