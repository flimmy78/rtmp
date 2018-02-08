#include <stdio.h>
#include <stdlib.h>
//#include <sys/types.h>
//#include <pthread.h>
//include <unistd.h>

#include "Livertmp.h"
//#include "flvparse.h"
#include "rtmp_push.h"


LiveRtmp::LiveRtmp()
{

}

LiveRtmp* LiveRtmp::createLiveRtmp()
{
   return new LiveRtmp();
}

int LiveRtmp::publishRtmp(char const* rtmpURL, int len)
{
	RTMPH264PUSHCTX *ctx = (RTMPH264PUSHCTX*) rtmp_get_h264_push_ctx();
    return ctx->rtmp_push_connect(rtmpURL);//"rtmp://baoqianli.8686c.com/live/123"
}

bool LiveRtmp::pushVideo(signed char* videoData, int videoSize , uint64_t pts_us )
{
	//struct timeval tv;
	//gettimeofday(&tv, NULL);

    RTMPH264PUSHCTX *ctx = (RTMPH264PUSHCTX*) rtmp_get_h264_push_ctx();
    return ctx->rtmp_h264_stream_push((uint8_t*)videoData, videoSize, pts_us );
}

bool LiveRtmp::pushAudio(signed char* audioData, int audioSize, uint64_t pts_us )
{
	//struct timeval tv;
	//gettimeofday(&tv, NULL);

    RTMPH264PUSHCTX *ctx = (RTMPH264PUSHCTX*) rtmp_get_h264_push_ctx();
    return ctx->rtmp_aac_stream_push((uint8_t*)audioData, audioSize, pts_us);
}


void LiveRtmp::closeRtmp()
{
    RTMPH264PUSHCTX *ctx = (RTMPH264PUSHCTX*) rtmp_get_h264_push_ctx();
    ctx->rtmp_push_close();
	return;
}


LiveRtmp::~LiveRtmp()
{

}


