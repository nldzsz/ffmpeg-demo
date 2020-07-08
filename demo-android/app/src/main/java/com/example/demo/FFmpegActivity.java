package com.example.demo;

import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.MenuItem;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import com.example.demo.Common.DDlog;
import com.example.demo.Common.PathTool;
import com.example.demo.FFmpegTest.FFmpegTest;
import com.kaopiz.kprogresshud.KProgressHUD;

import java.io.File;

public class FFmpegActivity extends AppCompatActivity implements View.OnClickListener {

    private Spinner mTestSpinner;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_ffmpeg);

        addBackButton();

        mTestSpinner = (Spinner) findViewById(R.id.splinner);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, getResources().getStringArray(R.array.test_items));
        mTestSpinner.setAdapter(adapter);
    }

    private void addBackButton() {
        ActionBar actionBar = getSupportActionBar();
        if(actionBar != null){
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(@NonNull MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                this.finish(); // back button
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onResume() {
        super.onResume();
        FFmpegTest.testFFmpeg();
    }
    private boolean isProcessing = false;
    private Handler mMainHander = new Handler(Looper.getMainLooper());
    private KProgressHUD hud;

    @Override
    public void onClick(View v) {
        if (isProcessing) {
            Toast.makeText(this,R.string.waitting,Toast.LENGTH_LONG).show();
            return;
        }
        isProcessing = true;
        if (hud == null) {
            hud = KProgressHUD.create(this)
                    .setStyle(KProgressHUD.Style.SPIN_INDETERMINATE)
                    .setDetailsLabel(getResources().getString(R.string.processing))
                    .setMaxProgress(100);
        }
        hud.show();

        File extFile = getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
        if (!extFile.exists()) {
            extFile.mkdir();
        }
        String resourceDir = extFile.getAbsolutePath()+"/";
        switch (mTestSpinner.getSelectedItemPosition()) {
            case 0:
                new Thread(()->{
                    String pcmname = "test_441_f32le_2.pcm";
                    String pcmpath = resourceDir + pcmname;
                    String dstpath = resourceDir + "test_441_f32le_2.aac";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doEncodecPcm2aac(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 1:
                new Thread(()->{
                    String pcmname = "test_441_f32le_2.pcm";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doResample(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 2:
                new Thread(()->{
                    String pcmname = "test_441_f32le_2.pcm";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_441_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doResampleAVFrame(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 3:
                new Thread(()->{
                    String pcmname = "test_640x360_yuv420p.yuv";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test.yuv";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doScale(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 4:
                new Thread(()->{
                    String pcmname = "test-mp3-1.mp3";
                    String pcmpath = resourceDir+pcmname;
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doDemuxer(pcmpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 5:
                new Thread(()->{
                    String apcmname = "test-mp3-1.mp3";
                    String vpcmname = "test_1280x720_2.mp4";
                    String apcmpath = resourceDir+apcmname;
                    String vpcmpath = resourceDir+vpcmname;
                    String dstpath = resourceDir+"test.MOV";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,apcmname,apcmpath);
                    PathTool.copyAssetsToDst(FFmpegActivity.this,vpcmname,vpcmpath);

                    FFmpegTest.doMuxerTwoFile(apcmpath,vpcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 6:
                new Thread(()->{
                    String pcmname = "abc-test.h264";
//                    String pcmname = "test_1280x720_1.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doReMuxer(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 7:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doSoftDecode(pcmpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 8:
                new Thread(()->{
                    String pcmname = "test_441_f32le_2.aac";
                    String pcmpath = resourceDir+pcmname;
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doSoftDecode2(pcmpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 9:
                new Thread(()->{
                    String pcmname = "test_640x360_yuv420p.yuv";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doSoftEncode(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 10:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doHardDecode(pcmpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 11:
                new Thread(()->{
                    String pcmname = "test_640x360_yuv420p.yuv";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"3-test.h264";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doHardEncode(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 12:
                new Thread(()->{
                    String pcmname = "abc-test.h264";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doReMuxerWithStream(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 13:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"2-test_1280x720_3.mov";   // 这里后缀可以改成flv,ts,avi mov则生成对应的文件
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doExtensionTranscode(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 14:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-test_1280x720_3.mov";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doTranscode(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 15:
                new Thread(()->{
                    String pcmname = "11-test.mp4";
                    String pcmpath = resourceDir+pcmname;
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doEncodeMuxer(pcmpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 16:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"test_240_s32le_2.pcm";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doCut(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 17:
                new Thread(()->{
                    String pcmname1 = "ll.mpg";
                    String pcmname2 = "ll.mpg";
                    String pcmpath1 = resourceDir+pcmname1;
                    String pcmpath2 = resourceDir+pcmname2;
                    String dstpath = resourceDir+"1-merge_1.mpg";
//                    String pcmname1 = "test_1280x720_3.mp4";
//                    String pcmname2 = "test_1280x720_5.mp4";
//                    String pcmpath1 = resourceDir+pcmname1;
//                    String pcmpath2 = resourceDir+pcmname2;
//                    String dstpath = resourceDir+"1-merge_1.mp4";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname1,pcmpath1);
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname2,pcmpath2);

                    FFmpegTest.MergeTwo(pcmpath1,pcmpath2,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 18:
                new Thread(()->{
                    String pcmname1 = "test_1280x720_1.mp4";
                    String pcmname2 = "test_1280x720_3.mp4";
                    String pcmpath1 = resourceDir+pcmname1;
                    String pcmpath2 = resourceDir+pcmname2;
                    String dstpath = resourceDir+"1-merge_1.mp4";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname1,pcmpath1);
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname2,pcmpath2);

                    FFmpegTest.MergeFiles(pcmpath1,pcmpath2,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 19:
                new Thread(()->{
                    String pcmname1 = "est_1280x720_4.mp4";
                    String pcmname2 = "test_441_f32le_2.aac";   //test-mp3-1.mp3
                    String pcmpath1 = resourceDir+pcmname1;
                    String pcmpath2 = resourceDir+pcmname2;
                    String dstpath = resourceDir+"11_add_music.mp4";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname1,pcmpath1);
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname2,pcmpath2);

                    FFmpegTest.addMusic(pcmpath1,pcmpath2,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 20:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-doJpg_get%3d.jpg";//"1-doJpg_get.jpg";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doJpgGet(pcmpath,dstpath,true,1);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 21:
                new Thread(()->{
                    String pcmname = "1-doJpg_get%3d.jpg";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-dojpgToVideo.mp4";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doJpgToVideo(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 22:
                new Thread(()->{
                    String pcmname = "test-mp3-1.mp3";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-addaudiovolome-1.mp3";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doChangeAudioVolume(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 23:
                new Thread(()->{
                    String pcmname = "test-mp3-1.mp3";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-audiovolume-2.mp3";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doChangeAudioVolume2(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
            case 24:
                new Thread(()->{
                    String pcmname = "test_1280x720_3.mp4";
                    String pcmpath = resourceDir+pcmname;
                    String dstpath = resourceDir+"1-videoscale_1.mp4";
                    PathTool.copyAssetsToDst(FFmpegActivity.this,pcmname,pcmpath);

                    FFmpegTest.doVideoScale(pcmpath,dstpath);
                    isProcessing = false;
                    processFinish();
                }).start();
                break;
        }

    }

    private void processFinish() {
        mMainHander.post(()->{
            hud.dismiss();
            Toast.makeText(this,R.string.finish,Toast.LENGTH_LONG).show();
        });
    }
}
