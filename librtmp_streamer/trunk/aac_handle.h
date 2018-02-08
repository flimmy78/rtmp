#ifndef __AAC_HANDLE_H__
#define __AAC_HANDLE_H__

typedef struct
{
    short syncword;
    short id;
    short layer;
    short protection_absent;
    short profile;
    short sf_index;
    short private_bit;
    short channel_configuration;
    short original;
    short home;
    short emphasis;
    short copyright_identification_bit;
    short copyright_identification_start;
    short aac_frame_length;
    short adts_buffer_fullness;
    short no_raw_data_blocks_in_frame;
    short crc_check;
	
    /* control param */
    short old_format;
} adts_header;

typedef struct
{
    short adts_header_present;
    short sf_index;
    short object_type;
    short channelConfiguration;
    short frameLength;
}faacDecHandle;

void adts_parse_handle(char* accfile, adts_header* adts, int* bitrate, float* frames_per_sec, int* samplerate,int* channel);

#endif
