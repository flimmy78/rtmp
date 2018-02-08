
// This app splits an FLV file based on cue points located in a data file.
// It also separates the video from the audio.
// All the reverse_bytes action stems from the file format being big-endian
// and needing an integer to hold a little-endian version (for proper calculation).

#include "flvparse.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#undef   lseek
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#ifdef PLATFORM_IOS
#include <memory.h>
#include <string.h>
#endif

#if(RTMP_STREAM_POLL)
#pragma pack(1)


#define TAG_TYPE_AUDIO 8
#define TAG_TYPE_VIDEO 9
#define FLV_STREAM_SAVE_FILE 0

#define BUFF_CACHE_MIN     (3*1024*1024)
#define AV_SYNC_TIME       (10*1000)
#define ATAG_COMPASATE_LEN (1024)

typedef unsigned char ui32[4];
typedef unsigned char ui24[3];
typedef unsigned char ui16[2];

typedef unsigned char byte;
typedef unsigned int uint;
typedef uint FLVPREVTAGSIZE;

typedef struct {
	byte Signature[3];
	byte Version;
	byte Flags;
	uint DataOffset;
} FLVHEADER;

typedef struct {
	byte TagType;
	ui24 DataSize;
	ui24 Timestamp;
	uint Reserved;
} FLVTAG;


typedef struct _FLVCTX{
	FLVHEADER      flv_header;
	AVINFO         flv_avinfo;
	RTMP_LIST_PTR  vdata_cache_list;
	RTMP_LIST_PTR  adata_cache_list;
	void           *vmutex;
	void           *amutex;
	int            init;
	uint32_t       vcache_data_len;
	uint32_t       acache_data_len;
	#if (FLV_STREAM_SAVE_FILE)
	FILE           *fp;
	#endif
	void		   *fawait;
	void           *fvwait;
	uint32_t       vts_start;
	uint32_t       vts_end;
	//void           *atagdata;
	RTMP_LIST_PTR  adata_compa_list;
	BOOLEAN        flv_first_enter;
	BOOLEAN        async_self;
	BOOLEAN        async_wait_video;
}FLVCTX;

static FLVCTX flv_ctx{
	/*.flv_header =*/  {0,},
	/*.flv_avinfo=*/   {0,},
	/*.vdata_cache_list=*/0,
	/*.adata_cache_list=*/0,
	/*.vmutex=*/0,
	/*.amutex=*/0,
	/*.init=*/0,
	/*.vcache_data_len=*/0,
	/*.acache_data_len=*/0,
	#if (FLV_STREAM_SAVE_FILE)
	/*.fp=*/0,
	#endif
	/*.fawait=*/0,
	/*.fvwait=*/0,
	/*.vts_start=*/0,
	/*.vts_end=*/0,
	/*.atagdata=*/0,
	/*.flv_first_enter=*/0,
	/*.async_self=*/0,
	/*.async_wait_video=*/0,
};

#define  LIST_PEEK(LIST, VAL, MUX)    do{ \
	 if( NULL != LIST){ \
		 rtmplive_mutex_lock(MUX);\
		 LIST = rtmp_list_move_first(LIST,(void**)&VAL);\
		 rtmplive_mutex_unlock(MUX);\
	 } \
	}while(0);

#define  LIST_POP(LIST, VAL, MUX, TOTAL)    do{ \
 if( NULL != LIST){ \
   	 rtmplive_mutex_lock(MUX);\
   	 LIST = rtmp_list_pop_front( LIST,(void**)&VAL);\
	 TOTAL -= VAL->len;\
   	 rtmplive_mutex_unlock(MUX);\
 } \
}while(0);


#define  LIST_PUSH(LIST, VAL, MUX, TOTAL, LEN)    do{ \
{ \
   	 rtmplive_mutex_lock(MUX);\
   	 rtmp_list_append_ex(&LIST, (void*)VAL);\
	 TOTAL += LEN;\
   	 rtmplive_mutex_unlock(MUX);\
 } \
}while(0);


static uint32_t flv_get_pts()
{
    static uint32_t pts = 10;
	return(pts++);
}

static uint32_t flv_get_tick_count()
{
	struct timeval tb = {0};
	static uint32_t pre_tick = 0;
	uint32_t   cur_tick = 0;
   // clock_gettime( CLOCK_MONOTONIC, &timeNow);
    gettimeofday(&tb, NULL);
    
    cur_tick = tb.tv_sec * 1000000 + tb.tv_usec;
    
    //if(cur_tick == pre_tick){
	//	cur_tick += 1;
    //}
	pre_tick = cur_tick;
	
	return (cur_tick);

}

static void flv_tag_free(void* ptr)
{
    if(ptr == NULL) return;
	AVTAGDATA *tag = (AVTAGDATA*)ptr;

	if(tag->free == 1) return;
	
	if(NULL != tag && NULL != tag->data) {
		free(tag->data);
	}
	if(NULL != tag){
		free(tag);
	}
}


//reverse_bytes - turn a BigEndian byte array into a LittleEndian integer
static uint reverse_bytes(byte *p, char c) {
	int r = 0;
	int i;
	for (i=0; i<c; i++) 
		r |= ( *(p+i) << (((c-1)*8)-8*i));
	return r;
}

static int flv_parse_avconfig(int type,uint8_t data)
{
	int x;
	switch(type){
	case TAG_TYPE_AUDIO:
		x=data & 0xF0;
		x=x >> 4;
		switch (x){
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 14:
		case 15:
		   flv_ctx.flv_avinfo.afmt  = x;
		   break;
		default:
		   flv_ctx.flv_avinfo.afmt = -1;
		   break;
		}
		x=data &0x0C;
		x=x>>2;
		switch (x)
		{
		case 0:
		case 1:
		case 2:
		case 3:
			flv_ctx.flv_avinfo.asamplerate = x;
			break;
		default:
			flv_ctx.flv_avinfo.asamplerate = -1;
			break;
		}
		x=data&0x02;
		x=x>>1;
		switch (x)
		{
		case 0:
		case 1:
			flv_ctx.flv_avinfo.asamplebit = x;
			break;
		default:
			flv_ctx.flv_avinfo.asamplebit = -1;
			break;
		}
		x=data & 0x01;
		switch (x)
		{
		case 0:
		case 1:
			flv_ctx.flv_avinfo.achannel = x;
			break;
		default:
			flv_ctx.flv_avinfo.achannel = -1;
			break;
		}
		break;
	case TAG_TYPE_VIDEO:
		x=data & 0xF0;
		x=x>>4;
		switch (x)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		    flv_ctx.flv_avinfo.vframe = x;
		    break;
		default:
			flv_ctx.flv_avinfo.vframe = -1;
			break;
		}
		x=data & 0x0F;
		switch (x)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		    flv_ctx.flv_avinfo.vformat = x;
			break;
		default:
			flv_ctx.flv_avinfo.vformat = -1;
			break;
		}
		break;
	case 18:
		break;
	default:break;
	}
	return 0;
}
static int flv_parse_spspps(uint8_t *buf, 
	uint8_t *&sps, uint32_t &sps_len, uint8_t *&pps, uint32_t &pps_len)
{
    uint8_t *ptr = buf;
	ui16 spsbg_len, ppsbg_len;
	uint32_t cost = 0;
    if(buf == NULL){
		return(cost);
    }
	if(ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2]==0x00){
		ptr += 3;
	}else{
	    return(cost);
	}

	//jump AVCDecoderConfigurationRecord - 5 byte
	ptr+=5;

	//get sps length.
	if(*ptr++ == 0xE1){
		memcpy(spsbg_len, ptr, 2);
        
		ptr+=2;
		sps = ptr;
		sps_len = reverse_bytes((byte *)spsbg_len, sizeof(spsbg_len));
		ptr += sps_len;
		
	}else {
	   return(cost);
	}
	//get pps parse
	if(*ptr++ == 0x01){
		memcpy(ppsbg_len, ptr, 2);
		ptr+=2;
		pps = ptr;
		pps_len = reverse_bytes((byte *)ppsbg_len, sizeof(ppsbg_len));
		ptr += pps_len;	
	}else {
	    return(cost);
	}
	cost = (uint32_t)(ptr - buf);
	return cost;
}

static int flv_parse_h264_frame(uint8_t *buf, uint8_t *&data, uint32_t &data_len)
{
    uint32_t cost = 0;
	uint8_t *ptr = buf;
	ui32 databg_len = { 0, };
	
	if(buf == NULL){
	   return(cost);
	}

	//jump conposition time -3 byte
    /*if(ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2]==0x00){
	  ptr += 3;
    }else{
	  return(cost);
    }*/

    ptr += 3;

	// get h264 data length.
	memcpy(databg_len, ptr, 4);
	ptr+=4;
	data = ptr;
	data_len = reverse_bytes((byte *)databg_len, sizeof(databg_len));

	ptr += data_len;
	cost = (uint32_t)(ptr - buf);
	return (cost);
}

static int flv_parse_aac_configure(uint8_t *buf, uint8_t *&data, uint32_t &data_len)
{
    uint32_t cost = 0;
	uint8_t *ptr = buf;
	ui32 databg_len = { 0, };
	
	if(buf == NULL){
	   return(cost);
	}
  
	return (cost);
}

static int combine_aac_adts_header(
	uint8_t (&adts_header)[7],uint32_t frame_length, 
	uint32_t sample_rate, uint32_t sample_bit, uint32_t channel)
{
   uint8_t aac_type = 1; /*1-LC*/
   /* 0 - 96000   1-88200     2-64000    3-48000    4-44100   5-32000
      *  6-24000     7-22050     8-16000   9-2000      a-11025   b-8000
      */
   uint8_t rate_idx = 4;
   /*1 - mono    2-strero*/
   uint8_t channels = 2;

   uint32_t num_data_block = frame_length / 1024;

   memset(adts_header, 0, 7);

   frame_length += 7;

    //syncword aac begin flagThe bit string '1111 1111 1111' 
	adts_header[0] = 0xFF;
    /* Sync point continued over first 4 bits + static 4 bits
	  *  (ID-x, layer-xx, protection-x)
	  */
	adts_header[1] = 0xF9;
	   
    /* Object type over first 2 bits */
    adts_header[2] = (aac_type & 0x2) << 6;
    /* rate index over next 4 bits */
    adts_header[2] |= (rate_idx & 0xF) << 2;
	/*skip 1 bit for private bit*/
    /* channels over last 2 bits */
    adts_header[2] |= (channels & 0x4) >> 2;
	   
    /* channels continued over next 2 bits + 4 bits at zero */
    adts_header[3] = (channels & 0x3) << 6;
    /*skip 4bit  
         *  |original/copy-1bit|home-1bit|copyright_identification_bit-1bit
         *  |copyright_identification_start-1bit|
	  */
	
    /* frame size over last 2 bits */
	adts_header[3] |= (frame_length & 0x1800) >> 11;
    /* frame size continued over full byte */
    adts_header[4] = (frame_length & 0x1FF8) >> 3;
	/* frame size continued first 3 bits */
	adts_header[5] = (frame_length & 0x7) << 5;
	   
	/* buffer fullness (0x7FF for VBR) over 5 last bits*/
	adts_header[5] |= 0x1F;
    /* buffer fullness (0x7FF for VBR) continued over 6 first bits + 2 zeros
	* number of raw data blocks */
    adts_header[6] = 0xFC;// one raw data blocks .
   
    adts_header[6] |= num_data_block & 0x03; //Set raw Data blocks.
	return (7);
}

//flv data parse.
void flv_parse_metadata(uint8_t *buf, uint32_t buf_size, uint32_t &cost)
{
	uint8_t *ptr = buf, *ptrend = NULL, *prePTR=NULL;
	FLVTAG tag;
	FLVPREVTAGSIZE pts;
	uint  ts=0, ts_new=0, ts_offset=0, ptag=0;
	uint32_t tag_data_len = 0, v_parse_len =0 ;
	AVTAGDATA *tagdata = NULL;
	uint8_t tag_first_char;
	int count = 0;
	uint32_t frameno = flv_get_pts();

	uint8_t* out_audio_buff = NULL;
	uint32_t out_audio_len = 0;
	
	cost = 0;
#if (FLV_STREAM_SAVE_FILE)
	long wlen = 0;
	if(NULL == flv_ctx.fp){
	    flv_ctx.fp=fopen("/sdcard/flv.aac","wb");
	}
#endif
	//open the input file
	if (buf == NULL || buf_size <= sizeof(FLVTAG)) {
		return;
	}
	if(NULL == flv_ctx.vmutex || NULL == flv_ctx.amutex){
		RTMPLIVE_TRACE("%s, must invoke flv_parse_init first\n", __FUNCTION__);
	    return ;
	}

	ptrend = buf+ buf_size;
    if( flv_ctx.flv_first_enter &&
		0 != memcmp(flv_ctx.flv_header.Signature, "FLV", 3)){
		flv_ctx.flv_first_enter = FALSE;
		//capture the FLV file header
		memcpy((char *)&flv_ctx.flv_header, ptr, sizeof(FLVHEADER));
		ptr += reverse_bytes((byte *)&flv_ctx.flv_header.DataOffset, sizeof(flv_ctx.flv_header.DataOffset)); 
    }
	
	//process each tag in the file
	do {
		prePTR = ptr;
		//capture the PreviousTagSize integer
		pts = *((int*) ptr); ptr += 4;
		
		memcpy((char *)&tag, ptr, sizeof(FLVTAG));
		ptr += sizeof(FLVTAG);
		
		//dump the audio data to the output file
		tag_data_len = reverse_bytes((byte *)&tag.DataSize, sizeof(tag.DataSize));

		//set the tag value to select on
		ptag = tag.TagType;

		//if we are not past the end of file, process the tag
		if ((ptr + tag_data_len) <= ptrend &&( (ptrend - ptr) > 10)) {
			//process tag by type
			switch (ptag) {
				case TAG_TYPE_AUDIO:  //we only process like this if we are separating audio into an mp3 file
					tag_first_char = *ptr++;
					flv_parse_avconfig(ptag, tag_first_char);

					if(*ptr == A_PKT_CONFIG){
						//flv_parse_aac_configure(++ptr)
						#if (FLV_STREAM_SAVE_FILE)
						//wlen = fwrite(++ptr,1,(tag_data_len - 2),flv_ctx.fp);
				        #endif
						ptr += (tag_data_len - 1);
						RTMPLIVE_TRACE("%s, get audio frame A_PKT_CONFIG\n", __FUNCTION__);
						
					}else if(*ptr == A_PKT_RAWDATA){
						uint8_t adts_header[7]={0};
						combine_aac_adts_header(adts_header, (tag_data_len - 2),
							flv_ctx.flv_avinfo.asamplerate,
							flv_ctx.flv_avinfo.asamplebit,
							flv_ctx.flv_avinfo.achannel);
					    tagdata =(AVTAGDATA*) malloc(sizeof(AVTAGDATA));
						tagdata->data = (uint8_t *) malloc(tag_data_len+7);
						tagdata->len = tag_data_len - 2 + 7;
						memcpy(tagdata->data, adts_header, 7);
					    memcpy(tagdata->data+7 , ++ptr, (tag_data_len - 2));
						tagdata->pts = frameno;
						tagdata->free = 0;
						LIST_PUSH(flv_ctx.adata_cache_list, tagdata, flv_ctx.amutex, 
						             flv_ctx.acache_data_len, (tag_data_len - 2));
					
						#if (FLV_STREAM_SAVE_FILE)
						wlen = fwrite(tagdata->data,1,tagdata->len,flv_ctx.fp);
				        #endif
						ptr += (tag_data_len-2);
						#if 0
						if((flv_ctx.vcache_data_len > BUFF_CACHE_MIN)
							&& streamer_sem_get_count(flv_ctx.fawait, &count) >= 0 
							&& count <= 0 ){
						   streamer_sem_post(flv_ctx.fawait);
						}
						RTMPLIVE_TRACE("%s, get audio frame A_PKT_RAWDATA\n", __FUNCTION__);
						#endif
					}else{
					     RTMPLIVE_TRACE("%s: Unsupport audio format(%d)\r\n", tag_first_char);
						 ptr += (tag_data_len - 1);
					}
					
					break;
				case TAG_TYPE_VIDEO:
					tag_first_char = *ptr++;
					flv_parse_avconfig(ptag, tag_first_char);
	                v_parse_len = 0; tagdata = NULL;
					if(flv_ctx.flv_avinfo.vframe == V_FRAME_KEY && 
						flv_ctx.flv_avinfo.vformat == V_FORMAT_AVC &&
						*ptr == V_H264_SEQUENCE_HEADER){//sps pps
						    uint8_t *sps = NULL, *pps = NULL;
							uint32_t sps_len = 0, pps_len = 0;
						    v_parse_len = flv_parse_spspps(++ptr, sps, sps_len, pps, pps_len);
							if(v_parse_len > 0) {
								tagdata = (AVTAGDATA*)malloc(sizeof(AVTAGDATA));
								tagdata->type = V_FRAME_SPSPPS;
								tagdata->data = (uint8_t *) malloc(sps_len+1+4);
								tagdata->len = sps_len+4;
								tagdata->pts  = frameno;
								tagdata->data[0] = 0x00;tagdata->data[1] = 0x00;
								tagdata->data[2] = 0x00;tagdata->data[3] = 0x01;
								memcpy(tagdata->data + 4, sps, sps_len);
								LIST_PUSH(flv_ctx.vdata_cache_list, tagdata, flv_ctx.vmutex, 
						             flv_ctx.vcache_data_len, tagdata->len);
								
								tagdata = (AVTAGDATA*)malloc(sizeof(AVTAGDATA));
								tagdata->type = V_FRAME_SPSPPS;
								tagdata->data = (uint8_t *) malloc(pps_len+1+4);
								tagdata->len = pps_len+4;
								tagdata->pts  = frameno;
								tagdata->data[0] = 0x00;tagdata->data[1] = 0x00;
								tagdata->data[2] = 0x00;tagdata->data[3] = 0x01;
								memcpy(tagdata->data + 4, pps, pps_len);
								RTMPLIVE_TRACE("%s, get video SPSPPS frame \n", __FUNCTION__);
							}
					}else if(((flv_ctx.flv_avinfo.vframe == V_FRAME_KEY)
					    || (flv_ctx.flv_avinfo.vframe == V_FRAME_P))
					    && flv_ctx.flv_avinfo.vformat == V_FORMAT_AVC 
					    && *ptr == V_H264_NALU_HEADER){
						uint8_t * data = NULL;
						uint32_t len = 0;
						v_parse_len = flv_parse_h264_frame(++ptr, data, len);

						if(v_parse_len > 0){
							tagdata = (AVTAGDATA*)malloc(sizeof(AVTAGDATA));
							tagdata->data = (uint8_t *) malloc(len+1+4);
							tagdata->len = len+4;
							tagdata->pts  = frameno;
							tagdata->tick = flv_get_tick_count();
							tagdata->type = flv_ctx.flv_avinfo.vframe;
							tagdata->data[0] = 0x00;tagdata->data[1] = 0x00;
							tagdata->data[2] = 0x00;tagdata->data[3] = 0x01;
							memcpy(tagdata->data + 4, data, len);
							//RTMPLIVE_TRACE("%s, get video H264 frame avsync_flag= %d\n", __FUNCTION__,avsync_tick);
						}
					}else{
					    RTMPLIVE_TRACE("%s: Unsupport vedio format(%d)\r\n", __FUNCTION__,tag_first_char);
						ptr += (tag_data_len-1);
					}
					if(v_parse_len > 0 && tagdata != NULL){
						LIST_PUSH(flv_ctx.vdata_cache_list, tagdata, flv_ctx.vmutex, 
						             flv_ctx.vcache_data_len, tagdata->len);
						ptr += (tag_data_len - 2); 
						if((flv_ctx.vcache_data_len > BUFF_CACHE_MIN)
							&& rtmplive_sem_get_count(flv_ctx.fvwait, &count) >= 0
							&& count <= 0 ){
						   rtmplive_sem_post(flv_ctx.fvwait);
						}
					}
					break;
				default:
				//skip the data of this tag
				ptr += tag_data_len;
				break;
			}
		}else{//ptr pointer backward
		    ptr = prePTR;
			goto PARSEEND;
		}
	} while (ptr <= ptrend);
PARSEEND:
    cost = (uint32_t)(ptr - buf);
}

void *flv_get_vcache_data()
{
    AVTAGDATA * data=NULL, *nextdata = NULL;
	if(NULL == flv_ctx.vmutex){
		RTMPLIVE_TRACE("%s, must invoke flv_parse_init first\n", __FUNCTION__);
	    return NULL;
	}
	if(NULL == flv_ctx.vdata_cache_list){
		RTMPLIVE_TRACE("%s: vdata cache is null and wait\n", __FUNCTION__);
		if( flv_ctx.fvwait){
			rtmplive_sem_wait(flv_ctx.fvwait, 0);
		}
	}
	RTMPLIVE_TRACE("%s: ----video tag count (%d)----\n", __FUNCTION__,
		rtmp_list_get_count(flv_ctx.vdata_cache_list));

    if(NULL != flv_ctx.vdata_cache_list){
		LIST_POP(flv_ctx.vdata_cache_list, data, flv_ctx.vmutex,flv_ctx.vcache_data_len);
		
		if(data->type == V_FRAME_KEY || data->type == V_FRAME_P){
			 int count = 0;
			 flv_ctx.vts_start = data->pts;
			 LIST_PEEK(flv_ctx.vdata_cache_list, nextdata, flv_ctx.amutex);
			 if(NULL != nextdata){
			 	 flv_ctx.vts_end = nextdata->pts;
			 }else{
			     flv_ctx.vts_end = flv_ctx.vts_start ;
			 }
             RTMPLIVE_TRACE("--------%s: find sync video frame ts-%d tick(%d)--------\n", 
			 	__FUNCTION__, data->pts, data->tick);
			 if(flv_ctx.async_wait_video){
				if(rtmplive_sem_get_count(flv_ctx.fawait, &count) >= 0 && count <= 0 ){
				   rtmplive_sem_post(flv_ctx.fawait);
				}
			 }
			 	
		}
    }
    return (void*)data;
}

void *flv_get_acache_data()
{
    AVTAGDATA *data=NULL ,*atag = NULL;
	int  count = 0;
	static BOOLEAN sync_need = TRUE;
	static BOOLEAN sync_need_wait_video = TRUE;
	static struct timeval tb={0};
	struct timeval te={0};
	int total = 0x7fff;
	BOOLEAN audio_drop = FALSE;
	
	if(NULL == flv_ctx.amutex){
		RTMPLIVE_TRACE("%s, must invoke flv_parse_init first\n", __FUNCTION__);
	    return NULL;
	}
	
	if(TRUE == flv_ctx.async_wait_video){
        RTMPLIVE_TRACE("%s: ----wait video sync proc----\n", __FUNCTION__);
		if( flv_ctx.fawait){
			rtmplive_sem_wait( flv_ctx.fawait, 0);
		}
		flv_ctx.async_wait_video = FALSE;
	}
	RTMPLIVE_TRACE("%s: ----audio tag count (%d)----\n", __FUNCTION__,
		rtmp_list_get_count(flv_ctx.adata_cache_list));
	
	if(TRUE == flv_ctx.async_self){
	    while(NULL != flv_ctx.adata_cache_list){
			LIST_PEEK(flv_ctx.adata_cache_list, data, flv_ctx.amutex);
		    if (data->pts < flv_ctx.vts_start) {
				LIST_POP(flv_ctx.adata_cache_list, data, flv_ctx.amutex,flv_ctx.acache_data_len);
                if(data){
				   RTMPLIVE_TRACE("--------%s: warnning!!!!! move data to compt list (%d-%d)ts-%d------\n",
					 __FUNCTION__,  flv_ctx.vts_start, flv_ctx.vts_end, data->pts);
	               // flv_tag_free(data); data = NULL;
	               LIST_PUSH(flv_ctx.adata_compa_list, data, flv_ctx.amutex, 
							 flv_ctx.acache_data_len, 0);
                }
		    }else if (data->pts < flv_ctx.vts_end) {
			    RTMPLIVE_TRACE("--------%s: find sync audio frame (%d-%d) ts-%d------\n", 
					__FUNCTION__, flv_ctx.vts_start, flv_ctx.vts_end, data->pts);
			    LIST_POP(flv_ctx.adata_cache_list, data, flv_ctx.amutex,flv_ctx.acache_data_len);
				gettimeofday(&tb, NULL);
				flv_ctx.async_self = FALSE;
				break;
		    } else{
		        data = NULL;
			    LIST_POP(flv_ctx.adata_compa_list, data, flv_ctx.amutex,total);
				if(data){
 					data->free = 0;
 					RTMPLIVE_TRACE("--%s: audio rapid than video get compa data---\n",__FUNCTION__);
		        }else{
				    RTMPLIVE_TRACE("--%s: get compa data null ---\n", __FUNCTION__);
				}
				break;
		    }
	    }  
	}else{
	    int ms = 0,compa_cnt=0;
		if(NULL != flv_ctx.adata_cache_list){
		 	LIST_POP(flv_ctx.adata_cache_list, data, flv_ctx.amutex,flv_ctx.acache_data_len);
		}
		gettimeofday(&te, NULL);
		ms = ((te.tv_sec - tb.tv_sec) * 1000000 + (te.tv_usec - tb.tv_usec))/1000;
		if(ms > AV_SYNC_TIME){
			flv_ctx.async_self = TRUE;
		}

		if(data != NULL ){
			compa_cnt = rtmp_list_get_count(flv_ctx.adata_compa_list);
			while(compa_cnt >= 10){
			    LIST_POP(flv_ctx.adata_compa_list, atag, flv_ctx.amutex,total);
				if(NULL != atag){
					atag->free = 0;
					flv_tag_free(atag);
				}
				compa_cnt--;
				RTMPLIVE_TRACE("--%s: drop compa list data ---\n", __FUNCTION__);
			}
			data->free = 1;
			LIST_PUSH(flv_ctx.adata_compa_list, data, flv_ctx.amutex, 
							 flv_ctx.acache_data_len, 0);
			RTMPLIVE_TRACE("--%s: get noraml audio data (pts:%d) ---\n", __FUNCTION__,data->pts);
		}else{
		    RTMPLIVE_TRACE("--%s: get noraml data null ---\n", __FUNCTION__);
		}
		
	}
    return (void*)data;
}

static uint32_t flv_get_vcache_state()
{
	if(flv_ctx.vcache_data_len >= BUFF_CACHE_MIN){
		return (AV_BUF_FULL);
	}else if(flv_ctx.vcache_data_len > 0){
		return (AV_BUF_SOME);
	}
    return (AV_BUF_EMPTY);
}

static void flv_get_first_keyframe(uint8_t **buf,  uint32_t &len)
{
    uint8_t         *pbuf = NULL;
	AVTAGDATA       *data = NULL;
	RTMP_LIST_PTR   list  = NULL;
	 
FIND_AGAIN:
	list = rtmp_list_move_first( flv_ctx.vdata_cache_list, ((void**)&data));
	if(data->type == V_FRAME_KEY){
		 *buf =(uint8_t*) malloc(data->len);
		 len = data->len;
		 pbuf = *buf;
		 memcpy(pbuf, data->data, len);
		 return;
	}
	while(NULL != (list = rtmp_list_move_next(list, ((void**)&data)))){
		if(data->type == V_FRAME_KEY){
			 *buf =(uint8_t*) malloc(data->len);
			 len = data->len;
			 pbuf = *buf;
			 return;
		}
	}
	usleep(1000);
	RTMPLIVE_TRACE("%s: find key frame again..\n", __FUNCTION__);
    goto FIND_AGAIN;
}



static uint32_t flv_get_acache_state()
{
	if (flv_ctx.acache_data_len >= BUFF_CACHE_MIN) {
		return (AV_BUF_FULL);
	}
	else if (flv_ctx.acache_data_len > 0) {
		return (AV_BUF_SOME);
	}
	return (AV_BUF_EMPTY);
}

static void* flv_get_avcofig()
{
    return (void*)(&flv_ctx.flv_avinfo);
}

static int flv_parse_init()
{
    int i = 0,cnt = 0;
    if(++flv_ctx.init > 1){
		RTMPLIVE_TRACE("%s, have been initialized user=%d\n", __FUNCTION__, flv_ctx.init);
		return (0);
	}
	flv_ctx.flv_first_enter = TRUE;
	flv_ctx.async_wait_video = TRUE;
	flv_ctx.async_self = TRUE;
					
    if(NULL != flv_ctx.vdata_cache_list){
	    rtmp_list_remove_all(flv_ctx.vdata_cache_list, flv_tag_free);
	    flv_ctx.vdata_cache_list = NULL;
    }
	if(NULL != flv_ctx.adata_cache_list){
	    rtmp_list_remove_all(flv_ctx.adata_cache_list, flv_tag_free);
	    flv_ctx.adata_cache_list = NULL;
    }

	cnt = rtmp_list_get_count(flv_ctx.adata_compa_list);
	while(i++ < cnt){
		AVTAGDATA *atag = NULL;
		int total = 0x7fffffff;
		LIST_POP(flv_ctx.adata_compa_list, atag, flv_ctx.amutex,total);
		atag->free = 0;
		flv_tag_free(atag);
	}
	if (NULL != flv_ctx.adata_compa_list) {
		rtmp_list_remove_all(flv_ctx.adata_compa_list, flv_tag_free);
		flv_ctx.adata_compa_list = NULL;
	}
	
    if(NULL == flv_ctx.vmutex){
        flv_ctx.vmutex = rtmplive_mutex_create();
    }
	 if(NULL == flv_ctx.amutex){
        flv_ctx.amutex = rtmplive_mutex_create();
    }
	if(NULL ==  flv_ctx.fawait){
	   flv_ctx.fawait = rtmplive_sem_create(0, 1, "audio_data_wait");
	}
	if(NULL ==  flv_ctx.fvwait){
	   flv_ctx.fvwait = rtmplive_sem_create(0, 1, "video_data_wait");
	}
    RTMPLIVE_TRACE("%s, init flv ctx.\n", __FUNCTION__);
	return(0);
}

static int flv_parse_uninit()
{
     if(--flv_ctx.init > 0){
		RTMPLIVE_TRACE("%s, resource using, not free usr=%d\n", __FUNCTION__, flv_ctx.init);
		return (0);
	}
	
	if (NULL != flv_ctx.vdata_cache_list) {
		rtmp_list_remove_all(flv_ctx.vdata_cache_list, flv_tag_free);
		flv_ctx.vdata_cache_list = NULL;
    }
	if (NULL != flv_ctx.adata_cache_list) {
		rtmp_list_remove_all(flv_ctx.adata_cache_list, flv_tag_free);
		flv_ctx.adata_cache_list = NULL;
    }

	if (NULL != flv_ctx.adata_compa_list) {
		rtmp_list_remove_all(flv_ctx.adata_compa_list, flv_tag_free);
		flv_ctx.adata_compa_list = NULL;
    }
	
    if(NULL != flv_ctx.vmutex){
        rtmplive_mutex_destroy(flv_ctx.vmutex);
		flv_ctx.vmutex = NULL;
    }
	 if(NULL != flv_ctx.amutex){
        rtmplive_mutex_destroy(flv_ctx.amutex);
		flv_ctx.amutex = NULL;
    }
	#if (FLV_STREAM_SAVE_FILE)
	if(NULL != flv_ctx.fp){	
		fclose(flv_ctx.fp);
		flv_ctx.fp = NULL;
	}		
    #endif
	if(flv_ctx.fawait){
		rtmplive_sem_post(flv_ctx.fawait);
		usleep(100*1000);
		rtmplive_sem_destroy(flv_ctx.fawait);
		flv_ctx.fawait = NULL;
    }
	if(flv_ctx.fvwait){
		rtmplive_sem_post(flv_ctx.fvwait);
		usleep(100*1000);
		rtmplive_sem_destroy(flv_ctx.fvwait);
		flv_ctx.fvwait = NULL;
    }
	memset(&flv_ctx, 0, sizeof(flv_ctx));
	RTMPLIVE_TRACE("%s, free flv ctx\n", __FUNCTION__);
	return(0);
}

static FLVPARSECTX flv_parse_ctx = {
	flv_parse_init,
	flv_parse_uninit,
	flv_parse_metadata,
	flv_get_vcache_data,
	flv_get_vcache_state,
	flv_get_acache_data,
	flv_get_acache_state,
	flv_get_avcofig,
	flv_tag_free,
	flv_get_first_keyframe,
};

void* flv_get_parse_ctx()
{
    return (void*)(&flv_parse_ctx);
}

#endif //#if(RTMP_STREAM_POLL)
