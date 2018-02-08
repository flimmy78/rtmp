#include "config.h"

#if(RTMP_STREAM_PUSH)

#include <stdio.h>
#include <stdlib.h>
#include "rtmp_push.h"
#include "rtmp.h"   
#include "rtmp_sys.h"   
#include "amf.h"  
//#include "sps_decode.h"

#ifdef PLATFORM_IOS
#include <memory.h>
#include <string.h>
#endif


#ifdef WIN32     
#include <windows.h>  
#pragma comment(lib,"WS2_32.lib")   
#pragma comment(lib,"winmm.lib")  
#endif 


/*
	参考  video-file-format-spec-v10-1	
	E.4.3.1
	2 = Sorenson H.263
	3 = Screen video
	4 = On2 VP6
	5 = On2 VP6 with alpha channel
	6 = Screen video version 2
	7 = AVC

	E.4.2.1
	0 = Linear PCM, platform endian
	1 = ADPCM
	2 = MP3
	3 = Linear PCM, little endian
	4 = Nellymoser 16 kHz mono
	5 = Nellymoser 8 kHz mono
	6 = Nellymoser
	7 = G.711 A-law logarithmic PCM
	8 = G.711 mu-law logarithmic PCM
	9 = reserved
	10 = AAC
	11 = Speex
	14 = MP3 8 kHz
	15 = Device-specific sound

*/
#define VIDEO_H 960
#define VIDEO_W 1280 
#define VIDEO_CODECID_H264  7
#define AUDIO_CODECID_AAC   10

// <2017.02.07 ms 	调整 声音和视频的时间戳 目前在辅驾宝上测试情况:音频会比视频提前500ms
// 2017.02.08 		时间调整改成在上层 所有进来的音频帧先加上500ms
#define ADJ_AVSYNC 0  		


#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
#define BUFFER_SIZE 32768
#define GOT_A_NAL_CROSS_BUFFER BUFFER_SIZE+1
#define GOT_A_NAL_INCLUDE_A_BUFFER BUFFER_SIZE+2
#define NO_MORE_BUFFER_TO_READ BUFFER_SIZE+3

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

typedef struct _RTMPGLOBAL{
	uint8_t          *fname;
	RTMP             *rtmp;  
	RTMPMetadata     metadata;
	uint8_t          *fbuff;
	uint32_t         fbuff_size;
	uint8_t          *fbuff_tmp;
	uint32_t         nalhead_pos;
    struct timeval   prets;
	uint32_t         tick;
	BOOLEAN          aac_conf_send;
	uint64_t	 audio_start_time ;  
	uint64_t	 video_start_time ;
	uint8_t         first_keyframe_flag ; 
	uint16_t		frame_gop_counter ;
}RTMPGLOBAL;

static RTMPGLOBAL rtmp_push={
	/*.fname =*/ NULL,
	/*.rtmp =*/ NULL,
	/*.metadata=*/{0},
	/*.fbuff = */NULL,
	/*.fbuff_size = */BUFFER_SIZE,
	/*.fbuff_tmp = */NULL,
	/*.nalhead_pos = */0,
	/*.prets=*/{0},
	/*.tick=*/ 0,
	/*.aac_conf_send=*/ 0,
	
	/* audio_start_time */ 0,
	/* video_start_time */ 0 ,
	/* first_keyframe_flag*/ 0 ,
	/* frame_gop_counter*/ 0 ,
};

static int rtmp_push_connect(const char* url)  
{  
    memset(&rtmp_push, 0, sizeof(RTMPGLOBAL));
	
	rtmp_push.nalhead_pos=0;
	rtmp_push.fbuff_size = BUFFER_SIZE;
	rtmp_push.fbuff = (uint8_t*)malloc(BUFFER_SIZE);
	rtmp_push.fbuff_tmp=(uint8_t*)malloc(BUFFER_SIZE);
	
	socket_ctx_init();  
	rtmp_push.rtmp = RTMP_Alloc();
	RTMP_Init(rtmp_push.rtmp);

	if (RTMP_SetupURL(rtmp_push.rtmp,(char*)url) == FALSE){
		RTMP_Free(rtmp_push.rtmp);
		return false;
	}
	RTMP_EnableWrite(rtmp_push.rtmp);
	if (RTMP_Connect(rtmp_push.rtmp, NULL) == FALSE) {
		RTMP_Free(rtmp_push.rtmp);
		return false;
	} 
	if (RTMP_ConnectStream(rtmp_push.rtmp,0) == FALSE){
		RTMP_Close(rtmp_push.rtmp);
		RTMP_Free(rtmp_push.rtmp);
		return false;
	}
	return true;  
}  

static void rtmp_push_close()  
{  
	if(rtmp_push.rtmp)  {  
		RTMP_Close(rtmp_push.rtmp);  
		RTMP_Free(rtmp_push.rtmp);  
		rtmp_push.rtmp = NULL;  
	}  
	socket_ctx_uninit();
	if (rtmp_push.fbuff != NULL){  
		free(rtmp_push.fbuff);
	}  
	if (rtmp_push.fbuff_tmp != NULL){  
		free(rtmp_push.fbuff_tmp);
	}
	if(NULL != rtmp_push.metadata.Pps){
		free(rtmp_push.metadata.Pps);
		rtmp_push.metadata.Pps = NULL;
	}
	if(NULL != rtmp_push.metadata.Sps){
		free(rtmp_push.metadata.Sps);
		rtmp_push.metadata.Sps = NULL;
	}
} 

int rtmp_send_packet(uint32_t nPacketType,uint8_t *data,uint32_t size,uint32_t nTimestamp)  
{  
	RTMPPacket* packet;
	
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+size);
	memset(packet,0,RTMP_HEAD_SIZE);
	
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body,data,size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; //audio or video packet type.
	packet->m_nInfoField2 = rtmp_push.rtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size !=4)
	{
		//packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;

	int nRet =0;
	if (RTMP_IsConnected(rtmp_push.rtmp)){
		nRet = RTMP_SendPacket(rtmp_push.rtmp,packet,TRUE);
	}
	
	free(packet);
	return nRet;  
}  

static int rtmp_send_h264_spspps(uint8_t *pps,int pps_len,uint8_t * sps,int sps_len)
{
	RTMPPacket * packet=NULL;
	uint8_t    * body=NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE+1024);
	//RTMPPacket_Reset(packet);//reset packet state.
	memset(packet,0,RTMP_HEAD_SIZE+1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++]   = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i],sps,sps_len);
	i +=  sps_len;

	/*pps*/
	body[i++]   = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len) & 0xff;
	memcpy(&body[i],pps,pps_len);
	i +=  pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = rtmp_push.rtmp->m_stream_id;

	int nRet = RTMP_SendPacket(rtmp_push.rtmp,packet,TRUE);
	free(packet);   
	return nRet;
}

int rtmp_send_h264_packet(uint8_t *data,uint32_t size,int bIsKeyFrame,uint32_t nTimeStamp)  
{  
    int i = 0, ret=0; 
	static int send_spspps_time = 0;
	if(data == NULL && size<11){  
		return false;  
	}  
	uint8_t *body = (uint8_t*)malloc(size+9);  
	memset(body,0,size+9);
	
	if(bIsKeyFrame){  
		body[i++] = 0x17;// 1:Iframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;  

		// NALU size   
		body[i++] = size>>24 & 0xff;  
		body[i++] = size>>16 & 0xff;  
		body[i++] = size>>8  & 0xff;  
		body[i++] = size     & 0xff;
		// NALU data   
		memcpy(&body[i],data,size);  
		/*if((send_spspps_time++ % 3600) == 0)*/
		{
			rtmp_send_h264_spspps(rtmp_push.metadata.Pps,rtmp_push.metadata.nPpsLen,
									rtmp_push.metadata.Sps,rtmp_push.metadata.nSpsLen);
		}
	}else{  
		body[i++] = 0x27;// 2:Pframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;  
		body[i++] = 0x00;  
		body[i++] = 0x00;  

		// NALU size   
		body[i++] = size>>24 & 0xff;  
		body[i++] = size>>16 & 0xff;  
		body[i++] = size>>8  & 0xff;  
		body[i++] = size     & 0xff;
		// NALU data   
		memcpy(&body[i],data,size);  
	}  
	ret = rtmp_send_packet(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);  
	free(body);  
	return (ret);  
} 

static int mfile_read_buffer(uint8_t *buf, int buf_size )
{
    static FILE *fp_send = NULL;
	static uint8_t*back_fname = NULL;
	
	if(NULL == rtmp_push.fname){
		RTMPLIVE_TRACE("%s: fname param NULL error\n", __FUNCTION__);
		return (-1);
	}
	if(NULL == buf || 0 == buf_size){
		RTMPLIVE_TRACE("%s: read buf param NULL error or buf_size is 0 \n", __FUNCTION__);
		return(-1);
	}
	FP_RESET:
	if(NULL == back_fname){
		back_fname = (uint8_t*)malloc(128);
		memset(back_fname, 0, 128);
		memcpy(back_fname, rtmp_push.fname, 
			(strlen((char*)rtmp_push.fname)>128 ? 128:strlen((char*)rtmp_push.fname)));
		if(fp_send){
			fclose(fp_send);
			fp_send = NULL;
		}
		if(NULL == fp_send){
		 fp_send = fopen((const char*)rtmp_push.fname, "rb");
	    }
	}else{
	    if(0 != memcmp((char*)back_fname, rtmp_push.fname, strlen((char*)rtmp_push.fname))){
			free(back_fname);
			back_fname = NULL;
			goto FP_RESET;
	    }
	}
	
	if(!feof(fp_send)){
		int rsize=fread(buf,1,buf_size,fp_send);
		return rsize;
	}else{
	    if(fp_send){
		   fclose(fp_send);
		   fp_send = NULL;
		}
		if(NULL != back_fname){
			free(back_fname);
			back_fname = NULL;
		}
		return -1;
	}
}
static int mfile_read_first_nalu_frame(NaluUnit &nalu,
	int (*read_buffer)( uint8_t *buf, int buf_size)) 
{
	int naltail_pos=rtmp_push.nalhead_pos;
	memset(rtmp_push.fbuff_tmp,0,BUFFER_SIZE);
	while(rtmp_push.nalhead_pos < rtmp_push.fbuff_size){  
		//search for nal header
		if(rtmp_push.fbuff[rtmp_push.nalhead_pos++] == 0x00 && 
			rtmp_push.fbuff[rtmp_push.nalhead_pos++] == 0x00) 
		{
			if(rtmp_push.fbuff[rtmp_push.nalhead_pos++] == 0x01)
				goto gotnal_head;
			else 
			{
				//cuz we have done an i++ before,so we need to roll back now
				rtmp_push.nalhead_pos--;		
				if(rtmp_push.fbuff[rtmp_push.nalhead_pos++] == 0x00 && 
					rtmp_push.fbuff[rtmp_push.nalhead_pos++] == 0x01)
					goto gotnal_head;
				else
					continue;
			}
		}
		else 
			continue;

		//search for nal tail which is also the head of next nal
gotnal_head:
		//normal case:the whole nal is in this m_pFileBuf
		naltail_pos = rtmp_push.nalhead_pos;  
		while (naltail_pos<rtmp_push.fbuff_size){  
			if(rtmp_push.fbuff[naltail_pos++] == 0x00 && 
				rtmp_push.fbuff[naltail_pos++] == 0x00 ){  
				if(rtmp_push.fbuff[naltail_pos++] == 0x01){
					nalu.size = (naltail_pos-3)-rtmp_push.nalhead_pos;
					break;
				}else{
					naltail_pos--;
					if(rtmp_push.fbuff[naltail_pos++] == 0x00 &&
						rtmp_push.fbuff[naltail_pos++] == 0x01)
					{	
						nalu.size = (naltail_pos-4)-rtmp_push.nalhead_pos;
						break;
					}
				}
			}  
		}

		nalu.type = rtmp_push.fbuff[rtmp_push.nalhead_pos]&0x1f; 
		memcpy(rtmp_push.fbuff_tmp,rtmp_push.fbuff+rtmp_push.nalhead_pos,nalu.size);
		nalu.data=rtmp_push.fbuff_tmp;
		rtmp_push.nalhead_pos=naltail_pos;
		return TRUE;   		
	}
    return FALSE;
}


static int mfile_read_one_nalu_frame(NaluUnit &nalu,
	int (*read_buffer)(uint8_t *buf, int buf_size))  
{    
	int naltail_pos = (int)rtmp_push.nalhead_pos;
	int ret;
	int nalustart;
	memset(rtmp_push.fbuff_tmp,0,BUFFER_SIZE);
	nalu.size=0;
	while(1){
		if(rtmp_push.nalhead_pos==NO_MORE_BUFFER_TO_READ){
			return FALSE;
		}
		while(naltail_pos < rtmp_push.fbuff_size){  
			//search for nal tail
			if(rtmp_push.fbuff[naltail_pos++] == 0x00 && 
				rtmp_push.fbuff[naltail_pos++] == 0x00){
				if(rtmp_push.fbuff[naltail_pos++] == 0x01){	
					nalustart=3;
					goto gotnal ;
				}else {
					//cuz we have done an i++ before,so we need to roll back now
					naltail_pos--;		
					if(rtmp_push.fbuff[naltail_pos++] == 0x00 && 
						rtmp_push.fbuff[naltail_pos++] == 0x01)
					{
						nalustart=4;
						goto gotnal;
					}
					else
						continue;
				}
			}else {
				continue;
			}

			gotnal:	
 				/**
				 *special case1:parts of the nal lies in a fbuff and we have to read from buffer 
				 *again to get the rest part of this nal
				 */
				if(rtmp_push.nalhead_pos==GOT_A_NAL_CROSS_BUFFER 
					|| rtmp_push.nalhead_pos==GOT_A_NAL_INCLUDE_A_BUFFER){
					nalu.size = nalu.size+naltail_pos-nalustart;
					if(nalu.size>BUFFER_SIZE){
						uint8_t *tmp=rtmp_push.fbuff_tmp;	//// save pointer in case realloc fails
						if((rtmp_push.fbuff_tmp = (uint8_t*)realloc(rtmp_push.fbuff_tmp,nalu.size)) ==  NULL )
						{
							free(tmp);  // free original block
							return FALSE;
						}
					}
					memcpy(rtmp_push.fbuff_tmp+nalu.size+nalustart-naltail_pos,rtmp_push.fbuff,naltail_pos-nalustart);
					nalu.data=rtmp_push.fbuff_tmp;
					rtmp_push.nalhead_pos=naltail_pos;
					return TRUE;
				}else {  //normal case:the whole nal is in this fbuff.
					nalu.type = rtmp_push.fbuff[rtmp_push.nalhead_pos]&0x1f; 
					nalu.size=naltail_pos-rtmp_push.nalhead_pos-nalustart;
					if(nalu.type==0x06){
						rtmp_push.nalhead_pos=naltail_pos;
						continue;
					}
					memcpy(rtmp_push.fbuff_tmp,rtmp_push.fbuff+rtmp_push.nalhead_pos,nalu.size);
					nalu.data=rtmp_push.fbuff_tmp;
					rtmp_push.nalhead_pos=naltail_pos;
					return TRUE;    
				} 					
		}

		if(naltail_pos>=rtmp_push.fbuff_size && rtmp_push.nalhead_pos!=GOT_A_NAL_CROSS_BUFFER 
			&& rtmp_push.nalhead_pos != GOT_A_NAL_INCLUDE_A_BUFFER){
			nalu.size = BUFFER_SIZE-rtmp_push.nalhead_pos;
			nalu.type = rtmp_push.fbuff[rtmp_push.nalhead_pos]& 0x1f; 
			memcpy(rtmp_push.fbuff_tmp,rtmp_push.fbuff+rtmp_push.nalhead_pos,nalu.size);
			if((ret=read_buffer(rtmp_push.fbuff,rtmp_push.fbuff_size))<BUFFER_SIZE)
			{
				memcpy(rtmp_push.fbuff_tmp+nalu.size,rtmp_push.fbuff,ret);
				nalu.size=nalu.size+ret;
				nalu.data=rtmp_push.fbuff_tmp;
				rtmp_push.nalhead_pos=NO_MORE_BUFFER_TO_READ;
				return FALSE;
			}
			naltail_pos=0;
			rtmp_push.nalhead_pos=GOT_A_NAL_CROSS_BUFFER;
			continue;
		}
		if(rtmp_push.nalhead_pos==GOT_A_NAL_CROSS_BUFFER || rtmp_push.nalhead_pos == GOT_A_NAL_INCLUDE_A_BUFFER){
			nalu.size = BUFFER_SIZE+nalu.size;
			uint8_t * tmp=rtmp_push.fbuff_tmp;	//// save pointer in case realloc fails
			if((rtmp_push.fbuff_tmp = (uint8_t*)realloc(rtmp_push.fbuff_tmp,nalu.size))== NULL)
			{
				free( tmp );  // free original block
				return FALSE;
			}
			memcpy(rtmp_push.fbuff_tmp+nalu.size-BUFFER_SIZE,rtmp_push.fbuff,BUFFER_SIZE);
			if((ret=read_buffer(rtmp_push.fbuff,rtmp_push.fbuff_size))<BUFFER_SIZE){
				memcpy(rtmp_push.fbuff_tmp+nalu.size,rtmp_push.fbuff,ret);
				nalu.size=nalu.size+ret;
				nalu.data=rtmp_push.fbuff_tmp;
				rtmp_push.nalhead_pos=NO_MORE_BUFFER_TO_READ;
				return FALSE;
			}
			naltail_pos=0;
			rtmp_push.nalhead_pos=GOT_A_NAL_INCLUDE_A_BUFFER;
			continue;
		}
	}
	return FALSE;  
} 

#if 0
static int rtmp_h264_mediafile_push(const char* fname)  
{    
	int ret;
	uint32_t now,last_update;
	  
	memset(&rtmp_push.metadata,0,sizeof(RTMPMetadata));
	memset(rtmp_push.fbuff,0,BUFFER_SIZE);

	if(NULL == fname || 0 ==strlen(fname)){
		return (FALSE);
	}
GET_NEW_MFILE:
	if (NULL == rtmp_push.fname){
		int flen = strlen(fname);
		rtmp_push.fname =(uint8_t*) malloc(flen+1);
		memset(rtmp_push.fname, 0, flen+1);
		memcpy(rtmp_push.fname, fname, flen);
	}else{
	    if (0 != memcmp(rtmp_push.fname, fname, strlen(fname))){
			free(rtmp_push.fname);
			rtmp_push.fname = NULL;
			goto GET_NEW_MFILE;
	    }
	} 
		
	if((ret=mfile_read_buffer(rtmp_push.fbuff,rtmp_push.fbuff_size))<0){
		return FALSE;
	}

	NaluUnit naluUnit;  
	
	mfile_read_first_nalu_frame(naluUnit,mfile_read_buffer);  
	rtmp_push.metadata.nSpsLen = naluUnit.size;  
	rtmp_push.metadata.Sps=NULL;
	rtmp_push.metadata.Sps=(uint8_t*)malloc(naluUnit.size);
	memcpy(rtmp_push.metadata.Sps,naluUnit.data,naluUnit.size);

	mfile_read_one_nalu_frame(naluUnit,mfile_read_buffer);  
	rtmp_push.metadata.nPpsLen = naluUnit.size; 
	rtmp_push.metadata.Pps=NULL;
	rtmp_push.metadata.Pps=(uint8_t*)malloc(naluUnit.size);
	memcpy(rtmp_push.metadata.Pps,naluUnit.data,naluUnit.size);
	
	int width = 0,height = 0, fps=0;  
	//h264_decode_sps(metaData.Sps,metaData.nSpsLen,width,height,fps);  
	//metaData.nWidth = width;  
	//metaData.nHeight = height;  
	if(fps){
		rtmp_push.metadata.nFrameRate = fps; 
	}else{
		rtmp_push.metadata.nFrameRate = 15;
	}

	uint32_t tick = 0;  
	uint32_t tick_gap = 1000/rtmp_push.metadata.nFrameRate; 
	mfile_read_one_nalu_frame(naluUnit,mfile_read_buffer);
	int bKeyframe  = (naluUnit.type == 0x05) ? TRUE : FALSE;
	int tdiff = 0;
	while(rtmp_send_h264_packet(naluUnit.data,naluUnit.size,bKeyframe,tick)) {    
got_sps_pps:
		RTMPLIVE_TRACE("NALU size:%8d\n",naluUnit.size);
		last_update=RTMP_GetTime();
		if(!mfile_read_one_nalu_frame(naluUnit,mfile_read_buffer)){
			RTMPLIVE_TRACE("file send finish end. \n");
			goto end;
		}
		if(naluUnit.type == 0x07 || naluUnit.type == 0x08){
			RTMPLIVE_TRACE("get sps and pps frame.. \n");
			goto got_sps_pps;
		}
		bKeyframe  = (naluUnit.type == 0x05) ? TRUE : FALSE;
		tick +=tick_gap;
		now=RTMP_GetTime();
		tdiff = now-last_update;
		if (tick_gap > tdiff){
			usleep((tick_gap - tdiff)*1000);  
		}
		RTMPLIVE_TRACE("time tramp:%8d %8d\n",tick_gap, tdiff);
		
	}  
	end:
	free(rtmp_push.metadata.Sps);
	free(rtmp_push.metadata.Pps);
	rtmp_push.metadata.Sps = NULL;
	rtmp_push.metadata.Pps = NULL;
	return TRUE;  
}  
#endif 

static void rtmp_send_h264_aac_metadata()
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


	{//video
		pos = AMF_EncodeNamedNumber(pos, end, &av_videocodecid, VIDEO_CODECID_H264);  
		pos = AMF_EncodeNamedNumber(pos, end, &av_width , VIDEO_W);
		pos = AMF_EncodeNamedNumber(pos, end, &av_height, VIDEO_H );
	}
	
	{//audio
		pos = AMF_EncodeNamedNumber(pos, end, &av_audiocodecid, AUDIO_CODECID_AAC); 
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
    packet.m_nInfoField2 	= rtmp_push.rtmp->m_stream_id;

    int iret = RTMP_SendPacket(rtmp_push.rtmp , &packet, 0);
	RTMPPacket_Free(&packet);	
}


static bool rtmp_h264_stream_push(uint8_t *buf, int buf_size, uint64_t pts_us)
{    
	int ret;
	NaluUnit nalu;  
 	
 
	int bKeyframe = 0;
	
    if(NULL == buf || 0== buf_size){
		RTMPLIVE_TRACE("%s: H264 data param error\n",__FUNCTION__);
		return true ;
    }
	//construct nalu unit
	nalu.type = (buf[0] & 0x1F);
	nalu.size = buf_size;
	nalu.data = buf;

	if(nalu.type == 0x7 ){
		if(NULL == rtmp_push.metadata.Sps){
			rtmp_push.metadata.nSpsLen = nalu.size;  
		    rtmp_push.metadata.Sps=(uint8_t*)malloc(nalu.size);
		    memcpy(rtmp_push.metadata.Sps,nalu.data,nalu.size);
		}
		return true;
	} else if(nalu.type == 0x8 ){
	    if(NULL == rtmp_push.metadata.Pps){
		    rtmp_push.metadata.nPpsLen = nalu.size; 
		    rtmp_push.metadata.Pps=(uint8_t*)malloc(nalu.size);
		    memcpy(rtmp_push.metadata.Pps,nalu.data,nalu.size);
	    }
		return true;
	}

	rtmp_push.metadata.nFrameRate = 15; 
	bKeyframe  = (nalu.type == 0x05) ? TRUE : FALSE;
	
	if(rtmp_push.first_keyframe_flag == 0){
		if(bKeyframe){
			rtmp_push.first_keyframe_flag = 1;
			RTMPLIVE_TRACE("--------------------------------------------");
			RTMPLIVE_TRACE(" ");
			RTMPLIVE_TRACE(" ");
			RTMPLIVE_TRACE("RTMP IDR KeyFrame ------------------------- ");
			RTMPLIVE_TRACE(" ");
			RTMPLIVE_TRACE(" ");
			RTMPLIVE_TRACE("--------------------------------------------");
			rtmp_send_h264_aac_metadata();
		}else{
			RTMPLIVE_TRACE("%s: first frame is not keyframe and lost\n",__FUNCTION__);
			return true ;
		}
    }

	// Only For Debug ++++ 
	if( bKeyframe ){
		if( rtmp_push.frame_gop_counter != 20 ){ // #define GOP 20
			RTMPLIVE_TRACE("GOP is %d " , rtmp_push.frame_gop_counter );
		}
		rtmp_push.frame_gop_counter = 1 ;
	}else{
		rtmp_push.frame_gop_counter ++ ;
	}
	// Only For Debug ----
	
	
	uint32_t rel_pts = 0  ;
	if( rtmp_push.video_start_time == 0 ){
		rtmp_push.video_start_time = pts_us ; 
		rel_pts = 0 ;
		RTMPLIVE_TRACE("rel_pts ");
	}else{
		rel_pts =  (pts_us - rtmp_push.video_start_time) / 1000 ;
		//RTMPLIVE_TRACE("video rel_pts %d " , rel_pts );
	}
	
	PACKET_PUSH:
	if(rtmp_send_h264_packet(nalu.data,nalu.size,bKeyframe,rel_pts)) {    
		if(bKeyframe){
			RTMPLIVE_TRACE("keyframe NALU size:%8d tickms=%ld\n", nalu.size, rel_pts);
		}else{
			RTMPLIVE_TRACE("send NALU size:%8d tickms=%ld\n", nalu.size, rel_pts);
		}
	}else{
		RTMPLIVE_TRACE("send NALU ERROR ");
		return false ;
	}  
	RTMPLIVE_TRACE("send video rel_pts %d " , rel_pts );
	return true;  
}  

static int rtmp_aac_codec_type(uint8_t(&specinfo)[2], int is_metadata)
{
    	//format: aac |sample rate 44khz|sample size 16bit|channel streto
    	//---1010---|-------11------|------1--------|-----1-----|         0xAE
	specinfo[0] = 0xAF; // 由于Android客户端固定要检查AF00开头 所以这里也改成AF 用VLC播放不受影响 
	specinfo[1] = (is_metadata? 0: 1);
	return 1;
}

#define AUDIO_CHANNEL  1 	//2:strereo 1:mono 


static int rtmp_aac_configure_send()
{    
	uint8_t specinfo[2];
	uint16_t audio_codec_config = 0;
	int i = 0, ret = 0; 
	#define AAC_CODEC_CONFIG_LEN  16
	uint8_t *body = (uint8_t*)malloc(AAC_CODEC_CONFIG_LEN);  
	
	rtmp_aac_codec_type(specinfo, true);
	body[i++] = specinfo[0];
	body[i++] = specinfo[1];

	audio_codec_config |= ((2<<11) & 0xf800);// 2:aac lc(low complexity)
	audio_codec_config |= ((4<<7) & 0x0780); //4:44khz
	audio_codec_config |= (( AUDIO_CHANNEL <<3 ) & 0x78);   //2:strereo 1:mono 
	audio_codec_config |= (0 & 0x7);         //padding:000

	body[i++] = (audio_codec_config >> 8 )&0xff;
	body[i++] = audio_codec_config & 0xff;  
	
	
	rtmplive_trace("rtmp_aac_configure_send ESDS:%04x  TAG:%04x" , audio_codec_config , *(uint16_t*)specinfo );
	
	ret = rtmp_send_packet(RTMP_PACKET_TYPE_AUDIO, body, i, 0);  
	free(body);  
	return(ret);
}


static bool rtmp_aac_stream_push(uint8_t *buf, int buf_size, uint64_t pts_us) // change to unsigned 
{    
	uint8_t specinfo[2];
	uint16_t audio_codec_config = 0;
	int i =0, ret = 0, body_size= buf_size+2; 
	
	if(rtmp_push.first_keyframe_flag == 0){
		RTMPLIVE_TRACE("%s: first keyframe not arrive, drop audio frame[for sync audio and video ]\n",__FUNCTION__);
		return true ;
    }
	
	uint32_t rel_pts = 0  ;
	if(  pts_us <  rtmp_push.video_start_time ){
		RTMPLIVE_TRACE("%s: audio frame timestamp %lu is earlier than first keyframe %lu , drop audio frame[for sync audio and video ]\n",
						__FUNCTION__ , pts_us , rtmp_push.video_start_time );
		return true;
	}else{
		rel_pts =  (pts_us - rtmp_push.video_start_time ) / 1000 + ADJ_AVSYNC ;
	}

	if(FALSE == rtmp_push.aac_conf_send){
		rtmp_aac_configure_send();
		rtmp_push.aac_conf_send = TRUE;
		RTMPLIVE_TRACE("rtmp_aac_configure_send !");
	}
		
	uint8_t *body = (uint8_t*)malloc(body_size);  
	
	rtmp_aac_codec_type(specinfo, false);
	body[i++] = specinfo[0];
	body[i++] = specinfo[1];
	
	//skip aac adts header
	//memcpy(body + i, buf + 7, buf_size - 7);
	memcpy(body+i, buf , buf_size);
	
	ret = rtmp_send_packet(RTMP_PACKET_TYPE_AUDIO, body, body_size, rel_pts );  
	RTMPLIVE_TRACE("send audio rel_pts %d " , rel_pts );
	free(body);  
	return ( ret > 0 )? true : false ;
}

static RTMPH264PUSHCTX rtmp_h264_push_ctx = {
	rtmp_push_connect,
	rtmp_h264_stream_push,
	rtmp_aac_stream_push,
	NULL,//rtmp_h264_mediafile_push, 
	rtmp_push_close,
};

void* rtmp_get_h264_push_ctx()
{
    return (void*)(&rtmp_h264_push_ctx);
}
#endif //RTMP_STREAM_PUSH
