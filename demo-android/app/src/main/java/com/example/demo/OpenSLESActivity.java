package com.example.demo;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.media.AudioFormat;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.demo.Audio.ADAudioMediaPlayer;
import com.example.demo.Audio.ADAudioTrackPlayer;
import com.example.demo.Audio.ADOpenSLES;
import com.example.demo.Audio.AudioPlayInterface;
import com.example.demo.Common.DDlog;
import com.example.demo.Common.PathTool;

import java.io.File;

public class OpenSLESActivity extends AppCompatActivity {

    private Spinner mTestSpinner;
    private AudioPlayInterface mPlayer;

    private static final String[] Test_items = {
            "MediaPlayer 播放音频 By RawId",
            "MediaPlayer 播放音频 By local",
            "MediaPlayer 播放音频 By https",
            "AduioTrack 播放音频",
            "OpenSL ES 播放音频"
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_opensles);
        addBackButton();

        mTestSpinner = (Spinner) findViewById(R.id.splinner);
        ArrayAdapter<String> adapter = new ArrayAdapter<>(this, android.R.layout.simple_list_item_1, Test_items);
        mTestSpinner.setAdapter(adapter);


        // 测试手机存储
//        PathTool.testPath(this);

//        Button btn = findViewById(R.id.button1);
//        btn.setOnTouchListener(yuntaiHoriCalListioner);
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

    private View.OnTouchListener yuntaiHoriCalListioner = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                Log.d("tag","ACTION_DOWN");
                return true;
            } else if(event.getAction() == MotionEvent.ACTION_UP || event.getAction() == MotionEvent.ACTION_CANCEL) {
                Log.d("tag","ACTION_UP");

            }
            Log.d("tag",event.toString());
            return false;
        }
    };

    @Override
    protected void onResume() {
        super.onResume();
    }


    public void onClickStart(View v) {
        switch (mTestSpinner.getSelectedItemPosition()) {
            case 0:{
                DDlog.logd("播放1 " + mTestSpinner.getSelectedItem());
                mPlayer = new ADAudioMediaPlayer(this,R.raw.test_mp3_1);
                mPlayer.play();
            }
            break;
            case 1:{
                DDlog.logd("播放2 " + mTestSpinner.getSelectedItem());
                mPlayer = new ADAudioMediaPlayer(this,"test_mp3.mp3");
                mPlayer.play();
            }
            break;
            case 2:{
                DDlog.logd("播放3 " + mTestSpinner.getSelectedItem());
                mPlayer = new ADAudioMediaPlayer(this, PathTool.getUriByString("https://img.flypie.net/test-mp3-1.mp3"));
                mPlayer.play();
            }
            break;
            case 3:{
                DDlog.logd("播放4 " + mTestSpinner.getSelectedItem());
                int bits = 8;
                if (bits == 8) {
                    // 小米 4 无法播放8位的音频数据
                    mPlayer = new ADAudioTrackPlayer(this,"test_441_s8_2.amr",44100,
                        AudioFormat.ENCODING_PCM_8BIT,AudioFormat.CHANNEL_OUT_STEREO,true);
                } else if (bits == 16) {
                    mPlayer = new ADAudioTrackPlayer(this,"test_441_s16le_2.amr",44100,
                        AudioFormat.ENCODING_PCM_16BIT,AudioFormat.CHANNEL_OUT_STEREO,false);
                } else if (bits == 17) {
                    mPlayer = new ADAudioTrackPlayer(this,"test_441_s16be_2.amr",44100,
                        AudioFormat.ENCODING_PCM_16BIT,AudioFormat.CHANNEL_OUT_STEREO,true);
                } else if (bits == 32) {
                    mPlayer = new ADAudioTrackPlayer(this,"test_441_f32le_2.amr",44100,
                        AudioFormat.ENCODING_PCM_FLOAT,AudioFormat.CHANNEL_OUT_STEREO,false);
                }

                mPlayer.play();
            }
            break;
            case 4:{
                DDlog.logd("播放5 " + mTestSpinner.getSelectedItem());
                File exFile = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
                final String cPath = exFile.getAbsolutePath() + "/audio.pcm";
                final int bits = 16;
                if (bits == 8) {
                    // 播放8位 音频数据 安卓机器不支持;和上面AudioTrack结果一样
                    mPlayer = new ADOpenSLES(cPath, 44100, 1, 0);
                } else if (bits == 16) {
                    // 播放16位 音频数据
                    mPlayer = new ADOpenSLES(cPath, 44100, 1, 1);
                } else if (bits == 32) {
                    // 播放32位 音频数据
                    mPlayer = new ADOpenSLES(cPath, 44100, 1, 2);
                }

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d("路径", "onCreate: " + cPath);
                        if (bits == 8) {
                            // 播放8位 音频数据
                            PathTool.copyAssetsToDst(OpenSLESActivity.this,"test_441_s8_2.amr",cPath);
                        } else if (bits == 16) {
                            // 播放16位 音频数据
                            PathTool.copyAssetsToDst(OpenSLESActivity.this,"test_441_s16le_2.amr",cPath);
                        } else if (bits == 32) {
                            // 播放32位 音频数据
                            PathTool.copyAssetsToDst(OpenSLESActivity.this,"test_441_s32le_2.amr",cPath);
                        }

                        // 拷贝完成后开始播放
                        mPlayer.play();
                    }
                }).start();
            }
            break;
            default:
                break;

        }
    }

    public void onClickStop(View v) {
        if (mPlayer != null) {
            mPlayer.stop();
            mPlayer = null;
        }
    }
}
