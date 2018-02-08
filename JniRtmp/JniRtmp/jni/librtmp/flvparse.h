#ifndef __FLV_PARSER___
#define __FLV_PARSER___

#include "config.h"
#include <sys/types.h>
#include "dataType.h"

#if (RTMP_STREAM_POLL)
#include "rtmp_list.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum{
	A_UNKNOWN                = -1,
	A_LINEPCM_BIGED          = 0,
	A_ADPCM                  = 1,
	A_MP3                    = 2,
	A_LINEPCM_LITTLEED       = 3,
	A_AAC                    = 10,
}AUDIOFMT;

typedef enum{
	V_UNKNOWN                = -1,
	V_JPEG                   = 1,
	V_H263                   = 2,
	V_H264                   = 7,
}VIDEOFMT;

typedef enum{
	AS_UNKNOWN                = -1,
	AS_5_5K                   = 0,
	AS_11K                    = 1,
	AS_22K                    = 2,
	AS_44K                    = 3,
	AS_AAC                    = 3,
}AUDIOSAMPLERATE;

typedef enum{
	A_SB_UNKNOWN             = -1,
	A_SB_8                   = 0,
	A_SB_16                  = 1,
}AUDIOSAMPLEBIT;
typedef enum{
	A_CH_UNKNOWN             = -1,
	A_CH_MONO                = 0,
	A_CH_STEREO              = 1,
	A_CH_AAC                 = 1,
}AUDIOCHANNEL;

typedef enum{
	V_FRAME_UNKNOWN          = -1,
	V_FRAME_KEY              = 1,
	V_FRAME_P                = 2,
	V_FRAME_INFO             = 5,
	V_FRAME_SPSPPS           = 0xF0,
}VIDEOFRAMETYPE;

typedef enum{
	V_FORMAT_UNKNOWN          = -1,
	V_FORMAT_VP6              = 4,
	V_FORMAT_VP6_ALPHA        = 5,
	V_FORMAT_AVC               =7,
}VIDEOFORMAT;

typedef enum{ 
	V_H264_UNKNOWN          = -1,
	V_H264_SEQUENCE_HEADER  = 0,
	V_H264_NALU_HEADER      = 1,
}H264HEADTYPE;

typedef enum{
	A_PKT_UNKNOWN  = -1,
	A_PKT_CONFIG   = 0,
    A_PKT_RAWDATA    =1,
}AACPKTTYPE;

typedef enum{
	AV_BUF_EMPTY  = 0,
	AV_BUF_SOME   = 1,
	AV_BUF_FULL   = 2,
}AVBUFSTATE;

typedef struct{
	uint32_t     pts;
	int          type;
	uint8_t      *data;
	uint32_t     len;
	uint32_t     tick;
	uint8_t      free;
}AVTAGDATA;

typedef struct{
	int          afmt;
    int          asamplerate;
	int          asamplebit;
	int          achannel;
    int          vformat;
	int          vframe;
}AVINFO;

typedef struct _FLVPARSECTX{
	int      (*flv_init)();
	int      (*flv_uninit)();
	void     (*flv_parse_metadata)(uint8_t *buf, uint32_t buf_size, uint32_t &cost);
	void*    (*flv_get_vcache_data)();
	uint32_t (*flv_get_vcache_state)();
	void*    (*flv_get_acache_data)();
	uint32_t (*flv_get_acache_state)();
	void*    (*flv_get_avconfig)();
	void     (*flv_tag_free)(void* tag);
	void     (*flv_get_first_keyframe)(uint8_t **buf,uint32_t &len);
}FLVPARSECTX;

void* flv_get_parse_ctx();

#ifdef __cplusplus
}
#endif

#endif//#if(RTMP_STREAM_POLL)

#endif//__FLV_PARSER___

