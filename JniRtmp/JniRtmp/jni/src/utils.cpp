
#include "utils.h"
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>


#define LOG_FILE "/sdcard/streamuser.txt"

#if (STREAMER_LOG_DISP)
static char log_buf[LOG_BUF_LEN]={0};
void streamer_trace(const char *text, ...)
{
	va_list params;
	va_start(params, text);
	vsnprintf((char *)log_buf, LOG_BUF_LEN - 1, (const char *)text, params);
	va_end (params);
	__android_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s", log_buf);
}
#endif

#if defined(WIN32) || defined(_WIN32)
void* streamer_sem_create(int initial_count, int max_count, const char* sem_name)
{
    HANDLE h;
	h = CreateEvent(NULL,TRUE,FALSE,NULL); 
	return(void*)h;
}

int streamer_sem_destroy(void* handle)
{
	if( NULL != handle)
	{
		return (CloseHandle( (HANDLE)handle ) )? 0 : -1;
	}
	return (0);
}

int streamer_sem_wait(void* handle, int time_out)
{
	if(NULL != handle){
	   if( 0 == time_out){
		   WaitForSingleObject((HANDLE)handle,INFINITE);			   
	   }else{
		   WaitForSingleObject ((HANDLE)handle,(DWORD)time_out);
	   }
	   
	   ResetEvent((HANDLE)handle);
	   return 0;
	}   
	return -1;
}

int streamer_sem_post(void* handle)
{
	if(NULL != handle)
		  return((SetEvent((HANDLE)handle)!=0)? 0:-1);
	return -1;
}

int streamer_sem_get_count(void* handle, int *count)
{
	if( NULL != handle){
		*count = -1;
		return 0;
	}
	return -1;

}

#else
void* streamer_sem_create(int initial_count, int max_count, const char* sem_name)
{
	sem_t *sem = NULL;

	sem = (sem_t*)malloc(sizeof(sem_t));
	if( 0 == sem_init(sem, 0, initial_count) )
	{
		STREAMER_TRACE((const char*)"streamer create sem success init = %d, sem_name = %s",initial_count, sem_name);
		return sem;
	}
	free(sem);
	STREAMER_TRACE((const char*)"streamer sem create error init = %d,sem_name=%s",initial_count, sem_name);
	return NULL;
}

int streamer_sem_destroy(void* handle)
{
	if( NULL != handle)
	{
		sem_destroy((sem_t*)handle);
		free(handle);
	}
	return (0);
}

int streamer_sem_get_count(void* handle, int *count)
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

int streamer_sem_post(void* handle)
{
	if(NULL != handle)
	{
		//STREAMER_TRACE((const char*)"streamer sem post  handle (%p)\n", handle);
        return sem_post((sem_t*)handle);
	}
	return (-1);
}

int streamer_sem_wait(void* handle, int time_out)
{
    if(NULL != handle){
        if( 0 == time_out){
        	return sem_wait((sem_t*)handle);
        }else{
        	struct timespec abs_timeout = {0};
        	abs_timeout.tv_sec = time(NULL) + time_out/1000;
        	return sem_timedwait((sem_t*)handle, &abs_timeout);
        }
    }
    return (-1);
}

void* streamer_mutex_create()
{
    pthread_mutex_t *mutex = NULL;
    mutex =(pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) );
    pthread_mutex_init(mutex,NULL);

    return (void*)(mutex);
}

void streamer_mutex_destroy(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_destroy((pthread_mutex_t*)mutex);
        free(mutex);
    }
}
void streamer_mutex_unlock(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_unlock((pthread_mutex_t*)mutex);
    }
}

void streamer_mutex_lock(void* mutex)
{
    if( NULL != mutex){
        pthread_mutex_lock((pthread_mutex_t*)mutex);
    }
}

void streamer_set_thread_priority(pthread_attr_t attr, int level)
{
	struct sched_param param; 
	pthread_attr_init(&attr); 
	pthread_attr_setschedpolicy(&attr, SCHED_RR); 
	param.sched_priority = level; 
	pthread_attr_setschedparam(&attr, &param); 
}
#endif


