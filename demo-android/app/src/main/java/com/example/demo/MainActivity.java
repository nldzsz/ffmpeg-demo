package com.example.demo;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button opengles = findViewById(R.id.button_opensles);
        opengles.setOnClickListener(onTapOpenglesButtonAction);

        Button ffmpeg = findViewById(R.id.button_ffmpeg);
        ffmpeg.setOnClickListener(onTapFFmepgButtonAction);

        requestPermission();
    }

    private View.OnClickListener onTapOpenglesButtonAction = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Intent intent = new Intent(MainActivity.this,OpenSLESActivity.class);
            startActivity(intent);
        }
    };

    private View.OnClickListener onTapFFmepgButtonAction = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Intent intent = new Intent(MainActivity.this,FFmpegActivity.class);
            startActivity(intent);
        }
    };


    //  ====== 权限申请 6.0以上要访问应用内目录必须要进行运行时权限申请======= //
    public static final int EXTERNAL_STORAGE_REQ_CODE = 10 ;
    public void requestPermission(){
        //判断当前Activity是否已经获得了该权限
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {

            //如果App的权限申请曾经被用户拒绝过，就需要在这里跟用户做出解释
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
                Toast.makeText(this, "please give me the permission", Toast.LENGTH_SHORT).show();
            } else {
                //进行权限请求
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.RECORD_AUDIO}, EXTERNAL_STORAGE_REQ_CODE);
            }
        }
        else {
            Log.d("测试", "已经有权限了");
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        switch (requestCode) {
            case EXTERNAL_STORAGE_REQ_CODE: {
                // 如果请求被拒绝，那么通常grantResults数组为空
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    //申请成功，进行相应操作

                } else {
                    //申请失败，可以继续向用户解释。
                }
                return;
            }
        }
    }
    //  ====== 权限申请 ======= //
}
