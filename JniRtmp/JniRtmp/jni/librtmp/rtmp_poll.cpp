#include "config.h"

#if(RTMP_STREAM_POLL)
#include <stdio.h>
#include <stdlib.h>
#include "rtmp_poll.h"
#include "rtmp.h"   
#include "rtmp_sys.h"   
#include "amf.h"  
#ifdef WIN32     
#include <windows.h>  
#pragma comment(lib,"WS2_32.lib")   
#pragma comment(lib,"winmm.lib")  
#endif 

#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
#define BUFFER_SIZE (1*1024*1024)
#define GOT_A_NAL_CROSS_BUFFER BUFFER_SIZE+1
#define GOT_A_NAL_INCLUDE_A_BUFFER BUFFER_SIZE+2
#define NO_MORE_BUFFER_TO_READ BUFFER_SIZE+3

#define RTMP_POLL_STREAM_SAVE_FILE  0

typedef struct _NaluUnit {  
	int      type;  
    int      size;  
	uint8_t  *data;  
}NaluUnit;
 
typedef struct _RTMPMetadata {  
	// video, must be h264 type   
	unsigned int    nWidth;  
	unsigned int    nHeight;  
	unsigned int    nFrameRate;      
	unsigned int    nSpsLen;  
	unsigned char   *Sps;  
	unsigned int    nPpsLen;  
	unsigned char   *Pps;   
} RTMPMetadata,*LPRTMPMetadata;  

enum  
{  
   VIDEO_CODECID_H264 = 7,  
};  

static int socket_ctx_init()    
{    
	#ifdef WIN32     
		WORD version;    
		WSADATA wsaData;    
		version = MAKEWORD(1, 1);    
		return (WSAStartup(version, &wsaData) == 0);    
	#else     
		return TRUE;    
	#endif     
}

static void socket_ctx_uninit()    
{    
	#ifdef WIN32     
	CleanupSockets();   
	WSACleanup();  
	#endif     
}    


typedef struct _RTMPPOLLGLOBAL{
	RTMP             *rtmp;  
	uint8_t          *fbuff;
	uint32_t         fbuff_size;
    struct timeval   prets;
	int64_t          tick;
}RTMPPOLLGLOBAL;

static RTMPPOLLGLOBAL rtmp_poll={
	/*.rtmp =*/ NULL,
	/*.fbuff = */NULL,
	/*.fbuff_size = */BUFFER_SIZE,
	/*.prets=*/{0},
	/*.tick=*/ 0,
};

static int rtmp_poll_connect(const char* url, int ulen)  
{  
    int bLiveStream = TRUE;
	
    if(NULL == url || ulen == 0){
		return (-1);
    }
	
	rtmp_poll.fbuff_size = BUFFER_SIZE;
	rtmp_poll.fbuff = (uint8_t*)malloc(BUFFER_SIZE);
	
	socket_ctx_init();  

	rtmp_poll.rtmp = RTMP_Alloc();
	RTMP_Init(rtmp_poll.rtmp);
	
	//set connection timeout,default 30s
	rtmp_poll.rtmp->Link.timeout=10;	
		
	// HKS's live URL
	//if(!RTMP_SetupURL(rtmp,"rtmp://live.hkstv.hk.lxdns.com/live/hks"))
	
	if(!RTMP_SetupURL(rtmp_poll.rtmp, (char*)url)){
		RTMPLIVE_TRACE("SetupURL Err\n");
		RTMP_Free(rtmp_poll.rtmp);
		socket_ctx_uninit();
		return (-1);
	}
	
	if (bLiveStream){
		rtmp_poll.rtmp->Link.lFlags |= RTMP_LF_LIVE;
	}
		
	//1hour
	RTMP_SetBufferMS(rtmp_poll.rtmp, 3600*1000);		
		
	if(!RTMP_Connect(rtmp_poll.rtmp,NULL)){
		RTMPLIVE_TRACE("Connect Err\n");
		RTMP_Free(rtmp_poll.rtmp);
		socket_ctx_uninit();
		return (-1);
	}
	
	if(!RTMP_ConnectStream(rtmp_poll.rtmp,0)){
		RTMPLIVE_TRACE("ConnectStream Err\n");
		RTMP_Close(rtmp_poll.rtmp);
		RTMP_Free(rtmp_poll.rtmp);
		socket_ctx_uninit();
		return (-1);
	}
	
	return true;  
}  

static void rtmp_poll_close()  
{  
	if(rtmp_poll.rtmp)  {  
		RTMP_Close(rtmp_poll.rtmp);  
		RTMP_Free(rtmp_poll.rtmp);  
		rtmp_poll.rtmp = NULL;  
	}  
	socket_ctx_uninit();
	if (rtmp_poll.fbuff != NULL){  
		free(rtmp_poll.fbuff);
	}  
} 

static int rtmp_poll_stream(uint8_t * &buf, uint32_t len, struct timeval &ts)
{
    int rlen = 0;
    static long total=0;
	
#if (RTMP_POLL_STREAM_SAVE_FILE)
    static FILE *fp = NULL;
	long wlen = 0;
	if(NULL == fp){
	    fp=fopen("/sdcard/receive1.flv","wb");
	}
#endif
    if(rlen = RTMP_Read(
		rtmp_poll.rtmp,
		/*rtmp_poll.fbuff*/(char*)buf,
		/*rtmp_poll.fbuff_size*/len)
	){
		#if (RTMP_POLL_STREAM_SAVE_FILE)
		//fseek(fp,0L,SEEK_END);
		wlen = fwrite(buf,1,rlen,fp);
        #endif
		total += rlen;
		//RTMPLIVE_TRACE("%s: %5dByte, Total: %5.2fkB\n",__FUNCTION__, rlen,total*1.0/1024);
	}
	return (rlen);
}

static RTMPH264POLLCTX rtmp_h264_poll_ctx = {
	rtmp_poll_connect,
	rtmp_poll_stream,
	rtmp_poll_close,
};

void* rtmp_get_h264_poll_ctx()
{
    return (void*)(&rtmp_h264_poll_ctx);
}
#endif