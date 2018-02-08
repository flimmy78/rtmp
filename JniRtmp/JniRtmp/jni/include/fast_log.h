/*
 * fast_log.h
 *
 *  Created on: 2016年10月24日
 *      Author: rd0394
 */

/*
	usage:

	#define LOG_TAG "your_tag_name"
	#include "fast_log.h"


	adb logcat -s "your_tag_name"

 * */

#ifndef FAST_LOG_H_
#define FAST_LOG_H_

#include <android/log.h>

#ifndef LOG_NDEBUG
	#ifdef NDEBUG
		#define LOG_NDEBUG 1	// no debug
	#else
		#define LOG_NDEBUG 0	// debug
	#endif
#endif

#ifndef LOG_TAG
#warning "you should define LOG_TAG in your source file. use default now!"
#define LOG_TAG "default"
#endif

#ifndef ALOG
#define ALOG(priority, tag, fmt...) \
    __android_log_print(ANDROID_##priority, tag, fmt)
#endif

#ifndef ALOGV
#if LOG_NDEBUG
#define ALOGV(...)   ((void)0)
#else
#define ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#endif
#endif

#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif


#endif /* FAST_LOG_H_ */
