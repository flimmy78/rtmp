
#ifndef __CONFIG_H_
#define __CONFIG_H_


#if defined(__LP64__) || \
        defined(__x86_64__) || defined(__ppc64__)
#define COMPILE_64BITS
#endif

#if defined(__APPLE__)
#define PLATFORM_IOS
#elif defined(__x86_64__)
#define PLATFORM_WIN
#elif defined(__GNUC__)
#define PLATFORM_ANDROID
#endif

#ifdef    PLATFORM_ANDROID
#define    ANDROID_HW_CODEC             1
#define    FFMPEG_AVCODEC_SUPPORT       1
#define    JPEG_TRUBO_SUPPORT           1
#define    RTMP_STREAM_PUSH             1
#define    RTMP_STREAM_POLL             1
#define    ANDROID_HW_AUDIO             1
#endif

#ifdef PLATFORM_IOS
#define    ANDROID_HW_CODEC             0
#define    FFMPEG_AVCODEC_SUPPORT       1
#define    JPEG_TRUBO_SUPPORT           1
#define    RTMP_STREAM_PUSH             1
#define    RTMP_STREAM_POLL             1
#define    ANDROID_HW_AUDIO             0
#endif


#endif //__CONFIG_H_

