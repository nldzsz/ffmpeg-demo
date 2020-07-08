cmake_minimum_required(VERSION 3.4.1)

# ding yi cpp yuan wen jian mulu bian liang
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src/main/cpp)
set(SRC_DIR2 ${PROJECT_SOURCE_DIR}/../../cppsrc)
set(SRC_DIR3 ${PROJECT_SOURCE_DIR}/../../cppcommon)
set(COMMON_DIR ${PROJECT_SOURCE_DIR}/../../)
set(OPENSLES_DIR ${SRC_DIR}/opensles)
set(libs "${CMAKE_SOURCE_DIR}/src/main/jniLibs")
set(FFMPEG_INC ${SRC_DIR}/ffmpeg)

# she zhi cpp yuan ma mu lu
aux_source_directory(${SRC_DIR} src_cpp)
aux_source_directory(${OPENSLES_DIR} opensles_cpp)
aux_source_directory(${SRC_DIR2} ffmpeg_demo)
aux_source_directory(${SRC_DIR3} ffmpeg_common)

# she zhi .h tou wen jian mu lu
include_directories(${COMMON_DIR})
include_directories(${OPENSLES_DIR})
include_directories(${FFMPEG_INC})
include_directories(${SRC_DIR2})
include_directories(${SRC_DIR3})

# yin ru android log ku
find_library( # Sets the name of the path variable.
              log-lib
              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

include_directories(${SRC_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -fexceptions -frtti")
# import ffmpeg library
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/src/main/jniLibs/${CMAKE_ANDROID_ARCH_ABI} -lm -lz")
# ding yi jiang gai ku da bao jin app de lei xing
add_library(adMedia SHARED
        ${src_cpp}
        ${ffmpeg_demo}
        ${ffmpeg_common}
        ${com_cpp}
        ${opensles_cpp}
        )
target_link_libraries(adMedia android log
        OpenSLES
        mediandk
        # for dynamic
        avformat avcodec avfilter avutil swresample swscale
        # for static
#        libavformat.a libavcodec.a libavfilter.a libavutil.a libswresample.a libswscale.a libfdk-aac.a libx264.a libmp3lame.a
        ${log-lib}
    )