#include <stdlib.h>
#include "stdio.h"
#include <unistd.h>

#include "RtmpPusher.h"
#include "H264Handle.h"

CRtmpPusher::CRtmpPusher(CBufferManager* buffMgr)
	:m_Stop(true),m_rtmpctx(NULL),m_VbuffMgr(buffMgr)
{
	m_rtmpctx = RTMP_Alloc();
	RTMP_Init(m_rtmpctx);

	memset(&metaData, 0, sizeof(metaData));

	memset(m_Name, 0x0, 128);
	sprintf(m_Name, "CBufferInfo_%p",this);
	m_VbuffMgr->Register(m_Name,this);
}

CRtmpPusher::~CRtmpPusher()
{
	m_VbuffMgr->UnRegister(m_Name);
	RTMP_Close(m_rtmpctx);
	RTMP_Free(m_rtmpctx);
}

int CRtmpPusher::Prepare(char* url)
{
	//rtmp://127.0.0.1/live/[streamname]
	RTMP_SetupURL(m_rtmpctx, url);
	RTMP_EnableWrite(m_rtmpctx);	
	
	if( RTMP_Connect(m_rtmpctx, 0) == 0)
	{
		fprintf(stderr, "call RTMP_Connect return 0\n");
		return 1;
	}

	if( RTMP_ConnectStream(m_rtmpctx, 0) == 0)
	{
		fprintf(stderr, "call RTMP_ConnectStream return 0\n");		
		return 2;
	}

	fprintf(stderr, "connect to %s m_rtmpctx:%p\n", url, m_rtmpctx);		
	return 0;
}
int CRtmpPusher::Start()
{
	if(!m_Stop)
	{
		fprintf(stderr, "CRtspServer is already start, call Start error!\n");
		return 1;
	}

	m_Stop = false;
	if(pthread_create(&m_pThread,NULL, ThreadRtmpPusherProcImpl,this))
	{
		fprintf(stderr, "ThreadRtspServerProcImpl false!\n");
		return -1;
	}
	return 0;
}

int CRtmpPusher::Pause()
{
	return 0;
}
int CRtmpPusher::Stop()
{
	if(m_Stop)
	{
		fprintf(stderr, "CRtspServer is stop, call stop error!\n");
		return -1;
	}

	m_Stop = true;
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;

	fprintf(stderr, "CRtspServer is stop\n");
	return 0;
}

void * CRtmpPusher::ThreadRtmpPusherProcImpl(void* arg)
{
	fprintf(stderr, "Call CRtmpPusher::ThreadRtmpPusherProcImpl!\n");
	CRtmpPusher* cRtmpPusher = (CRtmpPusher*)arg;

	cRtmpPusher->ThreadRtmpPusher();
	
	return NULL;
}

void CRtmpPusher::ThreadRtmpPusher()
{	
	#if 0
	uint64_t pts = 0;
	int length = 0;
	CH264Handle* h264Handle = new CH264Handle("test.264");
	unsigned char *frame = (unsigned char*)malloc(MAX_NALU_SIZE);

	fprintf(stderr,">>>>>>>>> %s %d %p\n", __func__, __LINE__, m_rtmpctx);
	SendMetadataPacket();
	fprintf(stderr,">>>>>>>>> %s %d\n", __func__, __LINE__);
	while(!m_Stop)
	{
		length = h264Handle->GetAnnexbFrame(frame, MAX_NALU_SIZE);
		if(length > 0)
		{
			if( RTMP_IsConnected(m_rtmpctx) == 0 || RTMP_IsTimedout(m_rtmpctx) != 0 )
			{
				fprintf(stderr, "rtmp[%d] is broke", m_rtmpctx->m_stream_id);	
				usleep(2*1000);
				continue;
			}
			fprintf(stderr, "h264Handle->GetAnnexbFrame length:%d\n", length);
			int type = frame[4] & 0x1f;
			if(type == 7)
			{
				metaData.nSpsLen = length - 4;
				if(metaData.Sps != NULL)
				{
					free(metaData.Sps);
				}
				metaData.Sps = (unsigned char*)malloc(length - 4);
				memcpy(metaData.Sps, frame + 4, length - 4);
			}
			else if(type == 8)
			{
				metaData.nPpsLen = length - 4;
				if(metaData.Pps != NULL)
				{
					free(metaData.Pps);
				}
				metaData.Pps = (unsigned char*)malloc(length - 4);
				memcpy(metaData.Pps, frame + 4, length - 4);
			}
			else if(type == 5 || type == 1)
			{
				Send264Packet(frame, length, pts);
				pts += 41;
				usleep(40000);
			}
		}
		else
		{
			usleep(2*1000);
		}
	}

	free(frame);
	delete h264Handle;
	#else

	SendMetadataPacket();

	int unHandleCnt = 0;
	CBuffer buffer;
	
	while(!m_Stop)
	{
		if( RTMP_IsConnected(m_rtmpctx) == 0 || RTMP_IsTimedout(m_rtmpctx) != 0 )
		{
			fprintf(stderr, "rtmp[%d] is broke", m_rtmpctx->m_stream_id);	
			usleep(2*1000);
			continue;
		}
		
		bool ret = usingQueue()->PopFront(buffer, unHandleCnt);
		int length = buffer.lenght;
		unsigned char *frame = buffer.data;
		uint64_t pts = buffer.pts;
		
		if(ret && length > 4)
		{
			fprintf(stderr, "usingQueue()->PopFront length:%d\n", length);
			int type = frame[4] & 0x1f;
			if(type == 7)
			{
				metaData.nSpsLen = length - 4;
				if(metaData.Sps != NULL)
				{
					free(metaData.Sps);
				}
				metaData.Sps = (unsigned char*)malloc(length - 4);
				memcpy(metaData.Sps, frame + 4, length - 4);
				fprintf(stderr, "get sps metaData.nSpsLen:%d\n", metaData.nSpsLen);
			}
			else if(type == 8)
			{
				metaData.nPpsLen = length - 4;
				if(metaData.Pps != NULL)
				{
					free(metaData.Pps);
				}
				metaData.Pps = (unsigned char*)malloc(length - 4);
				memcpy(metaData.Pps, frame + 4, length - 4);
				fprintf(stderr, "get pps metaData.nPpsLen:%d\n", metaData.nPpsLen);
			}
			else if(type == 5 || type == 1)
			{
				Send264Packet(frame, length, pts);
			}
		}
		else
		{
			usleep(2*1000);
		}
	}
	
	#endif
}

void CRtmpPusher::SendMetadataPacket()
{
	static const AVal av_setDataFrame = AVC("@setDataFrame");
	static const AVal av_onMetaData = AVC("onMetaData");
	static const AVal av_videocodecid = AVC("videocodecid");
	static const AVal av_width = AVC("width");
	static const AVal av_height = AVC("height");
	static const AVal av_audiocodecid = AVC("audiocodecid");
	static const AVal av_copyright = AVC("copyright");
	static const AVal av_company = AVC("www.protruly.com.cn");

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 512);
	char *pos = packet.m_body;
	char *end = packet.m_body + 512;

	pos = AMF_EncodeString(pos, end, &av_setDataFrame);
	pos = AMF_EncodeString(pos, end, &av_onMetaData);

	*pos ++ = AMF_OBJECT;

	{//copyright
		pos = AMF_EncodeNamedString(pos, end, &av_copyright, &av_company);
	}
	//if( m_pCodecs[2] )
	{//video
		pos = AMF_EncodeNamedNumber(pos, end, &av_videocodecid, 0x07);
		pos = AMF_EncodeNamedNumber(pos, end, &av_width , 1920);
		pos = AMF_EncodeNamedNumber(pos, end, &av_height, 960);
	}
	//if( m_pCodecs[3] )
	{//audio
		//pos = AMF_EncodeNamedNumber(pos, end, &av_audiocodecid, 0x0a);
	}

	*pos ++ = 0;
	*pos ++ = 0;
	*pos ++ = AMF_OBJECT_END;

    packet.m_packetType 	= RTMP_PACKET_TYPE_INFO;
    packet.m_nBodySize  	= pos - packet.m_body;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= 0;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 	= m_rtmpctx->m_stream_id;

	printf("SendMetadatas: len=%d\n", packet.m_nBodySize);
    int iret = RTMP_SendPacket(m_rtmpctx, &packet, 0);
	RTMPPacket_Free(&packet);	
}

void CRtmpPusher::Send264Config(RTMPMetadata meta)
{//264: index[2] + cts[3] + nalu.len[4] + nalu.index[1] + nalu.data[n]	
    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 2 + 3 + 5 + 1 + 2 + meta.nSpsLen + 1 + 2 + meta.nPpsLen);

    packet.m_packetType 	= RTMP_PACKET_TYPE_VIDEO;
    packet.m_nBodySize  	= 2 + 3 + 5 + 1 + 2 + meta.nSpsLen + 1 + 2 + meta.nPpsLen;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= 0;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 	= m_rtmpctx->m_stream_id;

	uint8_t *body = (uint8_t*)packet.m_body;
    int i = 0;
    body[i ++] = 0x17;
    body[i ++] = 0x00;
    body[i ++] = 0x00;
    body[i ++] = 0x00;
    body[i ++] = 0x00;

    //AVCDecoderConfigurationRecord
    body[i ++] = 0x01;
    body[i ++] = meta.Sps[1];
    body[i ++] = meta.Sps[2];
    body[i ++] = meta.Sps[3];
    body[i ++] = 0xff;

    //sps
    body[i ++] = 0xe1;
    body[i ++] = (meta.nSpsLen >> 8) & 0xff;
    body[i ++] = (meta.nSpsLen) & 0xff;
    memcpy(body + i, meta.Sps, meta.nSpsLen); i += meta.nSpsLen;
    
    //pps
    body[i ++] = 0x01;
    body[i ++] = (meta.nPpsLen >> 8) & 0xff;
    body[i ++] = (meta.nPpsLen) & 0xff;
    memcpy(body + i, meta.Pps, meta.nPpsLen); i += meta.nPpsLen;

	printf("Send264Config: sps=%d, pps=%d\n", meta.nSpsLen, meta.nPpsLen);
    int iret = RTMP_SendPacket(m_rtmpctx, &packet, 0);
	RTMPPacket_Free(&packet);
}

void CRtmpPusher::Send264Packet(unsigned char *frame, int length, long pts)
{//264: index[2] + cts[3] + nalu.len[4] + nalu.type[1] + nalu.data[n]
	uint8_t  type = frame[4] & 0x1f;
	if( metaData.Sps != NULL && metaData.Pps != NULL && type == 5) Send264Config(metaData);
	
	if( length < 4 ) return;
	uint8_t *data = frame + 4/*skip 0001*/; 
	int      size = length - 4;

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 9 + size);

    packet.m_packetType 	= RTMP_PACKET_TYPE_VIDEO;
    packet.m_nBodySize  	= 9 + size;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= pts;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_MEDIUM;
    packet.m_nInfoField2 	= m_rtmpctx->m_stream_id;

	memcpy(packet.m_body    , (data[0]&0x1f)!=1? "\x17\x01":"\x27\x01", 2);
    memcpy(packet.m_body + 2, "\x00\x00\x00", 3);
	AMF_EncodeInt32(packet.m_body + 5, packet.m_body + 9, size);
    memcpy(packet.m_body + 9, data, size);

	printf("Send264Packet: pts=%u, len=%d, nalu.type=%d\n", packet.m_nTimeStamp, size, (int)data[0]&0x1f);
    int iret = RTMP_SendPacket(m_rtmpctx, &packet, 1);
	RTMPPacket_Free(&packet);
}


