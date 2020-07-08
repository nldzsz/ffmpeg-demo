//
// Created by 飞拍科技 on 2019/5/23.
//

#ifndef MEDIA_ANDROID_OPENSLESCONTEXT_H
#define MEDIA_ANDROID_OPENSLESCONTEXT_H

// 标准的OpenSL ES库头文件
#include <SLES/OpenSLES.h>
// 针对安卓平台的扩展头文件(注意：如果要编写跨平台程序，则不能使用此头文件)
#include <SLES/OpenSLES_Android.h>
extern "C" {
#include "CLog.h"
};


/** 创建一个OpenSL ES的上下文
 * */
class OpenSLESContext {
private:

    /** SLObjectItf;OpenSL ES的引擎对象类型，它用于获取下面的引擎接口对象(SLEngineItf)。
     * 备注：OpenSL ES的其它功能组件也可能为此类型。
     * */
    SLObjectItf slObject;
    /** SLEngineItf;OpenSL ES的引擎接口对象类型，OpenSL ES各大功能组件(比如混音器，播放器等等)实例都通过此对象初始化
     * */
    SLEngineItf engineObject;
    bool  isInit;
    void initContext();

public:

    // 由于使用单利，这里私有构造方法
    OpenSLESContext();

    // 私有析构方法不能在类外部使用delete 对象;方式，会报错。
    virtual ~OpenSLESContext();

    // 获取引擎对象，必须在SLObjectItf对象实例化以后获取
    SLEngineItf getEngineObject();

    // 释放资源
    void releaseResources();
};


#endif //MEDIA_ANDROID_OPENSLESCONTEXT_H
