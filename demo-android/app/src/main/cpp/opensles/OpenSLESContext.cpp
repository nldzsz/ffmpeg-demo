//
// Created by 飞拍科技 on 2019/5/23.
//

#include "OpenSLESContext.h"

OpenSLESContext::OpenSLESContext() {
    LOGD("OpenSLESContext()");
    slObject = NULL;
    initContext();
}
OpenSLESContext::~OpenSLESContext() {
    LOGD("~OpenSLESContext()");
    releaseResources();
}

void OpenSLESContext::initContext() {
    // 用于OpenSL ES的各种操作结果;SL_RESULT_SUCCESS表示成功
    SLresult  result;

    // 1、创建OpenSL ES对象
    // OpenSL ES for Android is designed to be thread-safe,
    // so this option request will be ignored, but it will
    // make the source code portable to other platforms.
    SLEngineOption options[] = {{SL_ENGINEOPTION_THREADSAFE,SL_BOOLEAN_TRUE}};
    // 创建引擎对象
    result = slCreateEngine(&slObject,
                   0,
                   NULL,
                   0, // no interfaces
                   NULL,// no interfaces
                   NULL // no required
    );
    if (result != SL_RESULT_SUCCESS) {
        LOGD("slCreateEngine fail %lu",result);
    }

    // 2、实例化SL对象。就像声明了类变量，还需要初始化一个实例；阻塞方式初始化实例(第二个参数表示阻塞还是非阻塞方式)
    result = (*slObject)->Realize(slObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("Realize fail %lu",result);
    }

    // 3、获取引擎接口对象，必须在SLObjectItf对象实例化以后获取
    result = (*slObject)->GetInterface(slObject, SL_IID_ENGINE, &engineObject);
    if (result != SL_RESULT_SUCCESS) {
        LOGD("GetInterface fail %lu",result);
    }
}

SLEngineItf OpenSLESContext::getEngineObject() {
    return engineObject;
}

/** 释放 OpenSL ES资源，
 *  由于OpenSL ES的内存是由系统自动分配的，所以释放内存时只需要调用Destroy释放对应的SLObjectItf对象即可，然后它的属性ID接口对象直接置为NULL
 * */
void OpenSLESContext::releaseResources() {
    if (slObject != NULL) {
        (*slObject)->Destroy(slObject);
        slObject = NULL;
        engineObject = NULL;
    }
}

