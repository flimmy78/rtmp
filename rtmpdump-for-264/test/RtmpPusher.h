#ifndef RTMP_PUSHER_H
#define RTMP_PUSHER_H
#include <rtmp.h>
#include <pthread.h>
#include "BufferManager.h"

typedef struct _RTMPMetadata
{
	// video, must be h264 type   
	unsigned int    nWidth;
	unsigned int    nHeight;
	//unsigned int    nFrameRate;
	double    nFrameRate;
	unsigned int    nSpsLen;
	unsigned char   *Sps;
	unsigned int    nPpsLen;
	unsigned char   *Pps;
} RTMPMetadata, *LPRTMPMetadata;

class CRtmpPusher:public CBufferInfo
{
public:
	CRtmpPusher(CBufferManager* buffMgr);
	~CRtmpPusher();

	int Prepare(char* url);
	int Start();
	int Pause();
	int Stop();
	void SendMetadataPacket();
	void Send264Config(RTMPMetadata meta);
	void Send264Packet(unsigned char *frame, int length, long pts);

protected:
	static void* ThreadRtmpPusherProcImpl(void* arg);
	void ThreadRtmpPusher();
	
private:
	bool 			m_Stop;
	pthread_t 		m_pThread;
	RTMP*    		m_rtmpctx;
	RTMPMetadata 	metaData;
	char 			m_Name[128];
	CBufferManager* m_VbuffMgr;
};

#endif