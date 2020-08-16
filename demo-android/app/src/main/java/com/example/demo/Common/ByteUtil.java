package com.example.demo.Common;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class ByteUtil {
    /** 该类提供字节转换功能
     * java 默认只有小端字节序
     * */

    // 将byte[] 数组转换成short[]数组
    public static short[] bytesToShorts(byte[] bytes,int len,boolean isBe) {
        if(bytes==null){
            return null;
        }
        short[] shorts = new short[len/2];
        // 大端序
        if (isBe) {
            ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN).asShortBuffer().get(shorts);
        } else {
            ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
        }

        return shorts;
    }

    // 将byte[] 数组转换成float[]数组
    public static float[] bytesToFloats(byte[] bytes,int len,boolean isBe) {
        if(bytes==null){
            return null;
        }
        float[] floats = new float[len/4];
        // 大端序
        if (isBe) {
            ByteBuffer.wrap(bytes).order(ByteOrder.BIG_ENDIAN).asFloatBuffer().get(floats);
        } else {
            ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().get(floats);
        }

        return floats;
    }

    // 将short[] 转换为bytes[]
    public static byte[] shortsToBytes(short[] shorts) {
        if(shorts==null){
            return null;
        }
        byte[] bytes = new byte[shorts.length * 2];
        ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().put(shorts);

        return bytes;
    }

    // 将float[] 转换为bytes[]
    public static byte[] floatsToBytes(float[] floats) {
        if(floats==null){
            return null;
        }
        byte[] bytes = new byte[floats.length * 4];
        ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).asFloatBuffer().put(floats);

        return bytes;
    }

    static public String byte2hex(byte[] buffer) {
        String h = "";

        for (int i = 0; i < buffer.length; i++) {
            String temp = Integer.toHexString(buffer[i] & 0xFF);
            if (temp.length() == 1) {
                temp = "0" + temp;
            }
            h = h + " " + temp;
        }

        return h;

    }
}
