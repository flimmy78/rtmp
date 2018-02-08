#ifndef __LIVERTMP___
#define __LIVERTMP___

 

#include "rtmp_push.h"

class LiveRtmp{

public:
	LiveRtmp();
	virtual ~LiveRtmp();
	
	static LiveRtmp* createLiveRtmp();
    int publishRtmp(char const* rtmpURL, int len);   
    bool pushVideo(signed char* videoData, int videoSize , uint64_t pts_us );   
    bool pushAudio(signed char* audioData, int audioSize , uint64_t pts_us );       
    void closeRtmp();

}; 
 
#endif
