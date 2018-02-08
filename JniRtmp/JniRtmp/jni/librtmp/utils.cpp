
#include "utils.h"
#include "config.h"

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

#if (RTMPLIVE_LOG)
#define   LOG_BUF_LEN    (1024*1)
static char log_buf[LOG_BUF_LEN]={0};
void rtmplive_trace(const char *text, ...)
{
#ifdef PLATFORM_ANDROID
	va_list params;
	va_start(params, text);
	vsnprintf((char *)log_buf, LOG_BUF_LEN - 1, (const char *)text, params);
	va_end (params);
	__android_log_print(ANDROID_LOG_WARN, "rtmplive_jni", "%s", log_buf);
#else
    printf(text);
#endif
}
#endif

#if defined(WIN32) || defined(_WIN32)
#else
void* rtmplive_sem_create(int initial_count, int max_count, const char* sem_name)
{
	sem_t *sem = NULL;

	sem = (sem_t*)malloc(sizeof(sem_t));
	if( 0 == sem_init(sem, 0, initial_count) )
	{
		//STREAMER_TRACE((const char*)"streamer create sem success init = %d, sem_name = %s",initial_count, sem_name);
		return sem;
	}
	free(sem);
	//STREAMER_TRACE((const char*)"streamer sem create error init = %d,sem_name=%s",initial_count, sem_name);
	return NULL;
}

int rtmplive_sem_destroy(void* handle)
{
	if( NULL != handle)
	{
		sem_destroy((sem_t*)handle);
		free(handle);
	}
	return (0);
}

int rtmplive_sem_get_count(void* handle, int *count)
{
	if( NULL != handle){
		if( 0 != sem_getvalue((sem_t *)handle, count)){
			*count = -1;
		}else{
			//STREAMER_TRACE((const char*)"streamer sem count(%d) \n", *count);
		}
		return 0;
	}
	return (-1);
}

int rtmplive_sem_post(void* handle)
{
	if(NULL != handle)
	{
		//STREAMER_TRACE((const char*)"streamer sem post  handle (%p)\n", handle);
        return sem_post((sem_t*)handle);
	}
	return (-1);
}

int rtmplive_sem_wait(void* handle, int time_out)
{
    if(NULL != handle){
        if( 0 == time_out){
        	return sem_wait((sem_t*)handle);
        }else{
         #ifdef PLATFROM_ANDROID
        	struct timespec abs_timeout = {0};
        	abs_timeout.tv_sec = time(NULL) + time_out/1000;
        	return sem_timedwait((sem_t*)handle, &abs_timeout);
         #else
            return sem_wait((sem_t*)handle);

         #endif
        }
    }
    return (-1);
}

void* rtmplive_mutex_create()
{
    pthread_mutex_t *mutex = NULL;
    mutex =(pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) );
    pthread_mutex_init(mutex,NULL);

    return (void*)(mutex);
}

void rtmplive_mutex_destroy(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_destroy((pthread_mutex_t*)mutex);
        free(mutex);
    }
}
void rtmplive_mutex_unlock(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_unlock((pthread_mutex_t*)mutex);
    }
}

void rtmplive_mutex_lock(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_lock((pthread_mutex_t*)mutex);
    }
}


#endif


