#ifndef __UTILS___
#define __UTILS___

#ifdef __cplusplus
extern "C" {
#endif
#include "dataType.h"
#define  RTMPLIVE_LOG     1

void rtmplive_trace(const char *text, ...);

#if (RTMPLIVE_LOG)
#define RTMPLIVE_TRACE  rtmplive_trace
#else
#define RTMPLIVE_TRACE
#endif

#if defined(WIN32) || defined(_WIN32)
#else
#include <pthread.h>
void* rtmplive_sem_create(int initial_count, int max_count, const char* sem_name);
int rtmplive_sem_destroy(void* handle);
int rtmplive_sem_get_count(void* handle, int *count);
int rtmplive_sem_post(void* handle);
int rtmplive_sem_wait(void* handle, int time_out);

void* rtmplive_mutex_create();
void  rtmplive_mutex_destroy(void* mutex);
void  rtmplive_mutex_unlock(void* mutex);
void  rtmplive_mutex_lock(void* mutex);
	
#endif

#ifdef __cplusplus
}
#endif

#endif//__UTILS___