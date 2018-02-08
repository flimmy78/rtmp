#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "aac_handle.h"


#define AAC_MIN_STREAMSIZE 768
#define AAC_MAX_CHANNELS 2

static int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
static int adts_sample_channel[] = {1,2,3,4,5,6,7,8};
/* FAAD file buffering routines */
typedef struct {
    long bytes_into_buffer;
    long bytes_consumed;
    long file_offset;
	long bits_had_read;
    unsigned char *buffer;
    int at_eof;
    FILE *infile;
} aac_buffer;

void faacDecOpen(faacDecHandle *hDecoder)
{
	hDecoder->object_type = 1;
    hDecoder->channelConfiguration = 44100; /* Default: 44.1kHz */
    hDecoder->sf_index = 0;
    hDecoder->adts_header_present = 0;
    hDecoder->frameLength = 1024;
}

int fill_buffer(aac_buffer *b)
{
    int bread;
	
    if (b->bytes_consumed > 0)
    {
        if (b->bytes_into_buffer)
        {
            memmove((void*)b->buffer, (void*)(b->buffer + b->bytes_consumed),
                b->bytes_into_buffer*sizeof(unsigned char));
        }
		
        if (!b->at_eof)
        {
            bread = fread((void*)(b->buffer + b->bytes_into_buffer), 1,
                b->bytes_consumed, b->infile);
			
            if (bread != b->bytes_consumed)
                b->at_eof = 1;
			
            b->bytes_into_buffer += bread;
        }
		
        b->bytes_consumed = 0;
		
        if (b->bytes_into_buffer > 3)
        {
            if (memcmp(b->buffer, "TAG", 3) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 11)
        {
            if (memcmp(b->buffer, "LYRICSBEGIN", 11) == 0)
                b->bytes_into_buffer = 0;
        }
        if (b->bytes_into_buffer > 8)
        {
            if (memcmp(b->buffer, "APETAGEX", 8) == 0)
                b->bytes_into_buffer = 0;
        }
    }
	
    return 1;
}

void advance_buffer(aac_buffer *b, int bytes)
{
    b->file_offset += bytes;
    b->bytes_consumed = bytes;
    b->bytes_into_buffer -= bytes;
}

first_adts_analysis(unsigned char *buffer,adts_header* adts)
{
	if ((buffer[0] == 0xFF)&&((buffer[1] & 0xF6) == 0xF0))
	{
		adts->id = ((unsigned short) buffer[1] & 0x8) >> 3;
			fprintf(stderr, "adts:id  %d\n",adts->id);
		adts->layer = ((unsigned short) buffer[1] & 0x6) >> 1;
		    fprintf(stderr, "adts:layer  %d\n",adts->layer);
		adts->protection_absent = (unsigned short) buffer[1] & 0x1;
		    fprintf(stderr, "adts:protection_absent  %d\n",adts->protection_absent);
		adts->profile = ((unsigned short) buffer[2] & 0xc0) >> 6;
		    fprintf(stderr, "adts:profile  %d\n",adts->profile);
		adts->sf_index = ((unsigned short) buffer[2] & 0x3c) >> 2;
		    fprintf(stderr, "adts:sf_index  %d\n",adts->sf_index);
		adts->private_bit = ((unsigned short) buffer[2] & 0x2) >> 1;
		    fprintf(stderr, "adts:pritvate_bit  %d\n",adts->private_bit);
		adts->channel_configuration = ((((unsigned short) buffer[2] & 0x1) << 3) |
			(((unsigned short) buffer[3] & 0xc0) >> 6));
		    fprintf(stderr, "adts:channel_configuration  %d\n",adts->channel_configuration);
		adts->original = ((unsigned short) buffer[3] & 0x30) >> 5;
		    fprintf(stderr, "adts:original  %d\n",adts->original);
		adts->home = ((unsigned short) buffer[3] & 0x10) >> 4;
		    fprintf(stderr, "adts:home  %d\n",adts->home);
		adts->emphasis = ((unsigned short) buffer[3] & 0xc) >> 2;
		    fprintf(stderr, "adts:emphasis  %d\n",adts->emphasis);
		adts->copyright_identification_bit = ((unsigned short) buffer[3] & 0x2) >> 1;
		    fprintf(stderr, "adts:copyright_identification_bit  %d\n",adts->copyright_identification_bit);
		adts->copyright_identification_start = (unsigned short) buffer[3] & 0x1;
		    fprintf(stderr, "adts:copyright_identification_start  %d\n",adts->copyright_identification_start);
		adts->aac_frame_length = ((((unsigned short) buffer[4]) << 5) | 
			(((unsigned short) buffer[5] & 0xf8) >> 3));
		    fprintf(stderr, "adts:aac_frame_length  %d\n",adts->aac_frame_length);
		adts->adts_buffer_fullness = (((unsigned short) buffer[5] & 0x7) |
			((unsigned short) buffer[6]));
		    fprintf(stderr, "adts:adts_buffer_fullness  %d\n",adts->adts_buffer_fullness);
		adts->no_raw_data_blocks_in_frame = ((unsigned short) buffer[7] & 0xc0) >> 6;
		    fprintf(stderr, "adts:no_raw_data_blocks_in_frame  %d\n",adts->no_raw_data_blocks_in_frame);
		adts->crc_check = ((((unsigned short) buffer[7] & 0x3c) << 10) |
			(((unsigned short) buffer[8]) << 2) | 
			(((unsigned short) buffer[9] & 0xc0) >> 6));
		    fprintf(stderr, "adts:crc_check  %d\n",adts->crc_check);
	}
}

int adts_parse(aac_buffer *b, int *bitrate, float* frames_per_sec, int* samplerate,int* channel, float *length)
{
    int frames, frame_length;
    int t_framelength = 0;
    float bytes_per_frame;
	
    /* Read all frames to ensure correct time and bitrate */
    for (frames = 0; /* */; frames++)
    {
        fill_buffer(b);
		
        if (b->bytes_into_buffer > 7)
        {
            /* check syncword */
            if (!((b->buffer[0] == 0xFF)&&((b->buffer[1] & 0xF6) == 0xF0)))
                break;
			
			if (frames == 0)
			{
				*samplerate = adts_sample_rates[(b->buffer[2] & 0x3c) >> 2];
				*channel = adts_sample_channel[((b->buffer[3] & 0xC0) >> 6) & ((b->buffer[2] & 0x1) << 2)];
			}
                
			
            frame_length = ((((unsigned int)b->buffer[3] & 0x3)) << 11)
                | (((unsigned int)b->buffer[4]) << 3) | (b->buffer[5] >> 5);
			
            t_framelength += frame_length;
			
            if (frame_length > b->bytes_into_buffer)
                break;
			
            advance_buffer(b, frame_length);
        } else {
            break;
        }
    }
	
    *frames_per_sec = (float)(*samplerate)/1024.0f;
    if (frames != 0)
        bytes_per_frame = (float)t_framelength/(float)(frames*1000);
    else
        bytes_per_frame = 0;
    *bitrate = (int)(8. * bytes_per_frame * (*frames_per_sec) + 0.5);
	fprintf(stderr, "bitrate:  %d\n",*bitrate);
    if (frames_per_sec != 0)
        *length = (float)frames/(*frames_per_sec);
    else
        *length = 1;
	fprintf(stderr, "length  %f\n",*length);
	fprintf(stderr, "frames_per_sec:  %f\n", (*frames_per_sec));
	fprintf(stderr, "samplerate:  %d\n", *samplerate);
	fprintf(stderr, "channel:  %d\n", *channel);
    return 1;
}


#if 1
void adts_parse_handle(char* accfile, adts_header* adts, int* bitrate, float* frames_per_sec, int* samplerate,int* channel)
{
	int bread;
	int fileread;
	float length;
	aac_buffer b;

	memset(&b, 0, sizeof(aac_buffer));
    //b.infile = fopen("test.aac", "rb");
	//b.infile = fopen("fff.aac", "rb");
	b.infile = fopen(accfile, "rb");
    if (b.infile == NULL)
	{ 
  	    /* unable to open file */
 	    fprintf(stderr, "Error opening the input file");
	   
	} 
    fseek(b.infile, 0, SEEK_END);
    fileread = ftell(b.infile);
    fseek(b.infile, 0, SEEK_SET);

    /*buffer*/
	if (!(b.buffer = (unsigned char*)malloc(AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)))
    {
 	    fprintf(stderr, "Memory allocation error\n");
 	
	} 
	memset(b.buffer, 0, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS);
    /*read*/

	bread = fread(b.buffer, 1, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS, b.infile);
	b.bytes_into_buffer = bread;
    b.bytes_consumed = 0;
    b.file_offset = 0;
	b.bits_had_read = 0;

	if (bread != AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)
        b.at_eof = 1;

	if ((b.buffer[0] == 0xFF) && ((b.buffer[1] & 0xF6) == 0xF0))
    {
 	    first_adts_analysis(b.buffer,adts);
		b.bits_had_read = 74;
	    adts_parse(&b, bitrate, frames_per_sec, samplerate, channel, &length);
		fseek(b.infile, 0, SEEK_SET);
		
		bread = fread(b.buffer, 1, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS, b.infile);
		if (bread != AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)
            b.at_eof = 1;
        else
            b.at_eof = 0;
        b.bytes_into_buffer = bread;
        b.bytes_consumed = 0;
        b.file_offset = 0;
	}

	/* Channel Configuration */
	 	 
    /*close*/
    fclose(b.infile);
}
#else
void main()
{
	int bread;
	int samplerate;
	int fileread;
	int bitrate;
	float length;
	aac_buffer b;
	adts_header adts;
	faacDecHandle hDecoder;

	memset(&b, 0, sizeof(aac_buffer));
    //b.infile = fopen("test.aac", "rb");
	//b.infile = fopen("fff.aac", "rb");
	b.infile = fopen("HK.aac", "rb");
    if (b.infile == NULL)
	{ 
  	    /* unable to open file */
 	    fprintf(stderr, "Error opening the input file");
	   
	} 
    fseek(b.infile, 0, SEEK_END);
    fileread = ftell(b.infile);
    fseek(b.infile, 0, SEEK_SET);

    /*buffer*/
	if (!(b.buffer = (unsigned char*)malloc(AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)))
    {
 	    fprintf(stderr, "Memory allocation error\n");
 	
	} 
	memset(b.buffer, 0, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS);
    /*read*/

	bread = fread(b.buffer, 1, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS, b.infile);
	b.bytes_into_buffer = bread;
    b.bytes_consumed = 0;
    b.file_offset = 0;
	b.bits_had_read = 0;

	if (bread != AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)
        b.at_eof = 1;
    //faacDecOpen(&hDecoder);

	if ((b.buffer[0] == 0xFF) && ((b.buffer[1] & 0xF6) == 0xF0))
    {
 	    first_adts_analysis(b.buffer,&adts);
		b.bits_had_read = 74;
	    adts_parse(&b, &bitrate, &length);
		fseek(b.infile, 0, SEEK_SET);
		
		bread = fread(b.buffer, 1, AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS, b.infile);
		if (bread != AAC_MIN_STREAMSIZE*AAC_MAX_CHANNELS)
            b.at_eof = 1;
        else
            b.at_eof = 0;
        b.bytes_into_buffer = bread;
        b.bytes_consumed = 0;
        b.file_offset = 0;
	}

	//if(hDecoder.object_type == 23)
 //       hDecoder.frameLength >>= 1;
	//fprintf(stderr, "encode frame length:  %d\n",hDecoder.frameLength);

	/* Channel Configuration */
	 	 
    /*close*/
    fclose(b.infile);
}
#endif