#ifndef __RTMP_POLL_H__
#define __RTMP_POLL_H__

#include "config.h"
#include "utils.h"

#if(RTMP_STREAM_POLL)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _RTMPH264POLLCTX{
	int    (*rtmp_poll_connect)(const char* url, int len);
	int    (*rtmp_poll_stream)(uint8_t * &buff, uint32_t len, struct timeval &ts);
	void   (*rtmp_poll_close)();
}RTMPH264POLLCTX;
extern void* rtmp_get_h264_poll_ctx();
#ifdef __cplusplus
}
#endif

#endif //RTMP_STREAM_POLL

#endif//__RTMP_POLL_H__

