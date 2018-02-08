/**
 * Simplest Librtmp Send 264
 *
 * �����裬����
 * leixiaohua1020@126.com
 * zhanghuicuc@gmail.com
 * �й���ý��ѧ/���ֵ��Ӽ���
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * ���������ڽ��ڴ��е�H.264����������RTMP��ý���������
 * This program can send local h264 stream to net server as rtmp live stream.
 */

#include <stdio.h>
#include <pthread.h>
#include "librtmp_send264.h"
#include "ringfifo.h"

FILE *fp_h264;
FILE *fp_AAC;
pthread_t ntid;

//���ļ��Ļص�����
//we use this callback function to read data from buffer
int read_buffer0(unsigned char *buf, int buf_size){
//	if (!feof(fp_send1)){
//		int true_size = fread(buf, 1, buf_size, fp_send1);
//		return true_size;
//	}
//	else{
//		fseek(fp_send1, 0L, SEEK_SET);
//		return 0;
//	}
	//printf("read_buffer1\n");
	struct ringbuf getinfo;
	if(ringget(&getinfo) > 0)
	{
		memcpy(buf,getinfo.buffer,getinfo.size);
		return getinfo.size;
	}
	else
	{
		return 0;
	}
}

int read_h264(unsigned char *buf, int buf_size){
	int true_size = fread(buf, 1, buf_size, fp_h264);
	if(true_size <= 0)
	{
		fseek(fp_h264, 0L, SEEK_SET);
		true_size = fread(buf, 1, buf_size, fp_h264);
	}
	return true_size;
}

int read_AAC(unsigned char *buf, int buf_size){
	unsigned char aac_header[7];
	int true_size = fread(aac_header, 1, 7, fp_AAC);
	if(true_size <= 0)
	{
		fseek(fp_AAC, 0L, SEEK_SET);
		true_size = fread(aac_header, 1, 7, fp_AAC);
	}
	unsigned int body_size = *(unsigned int *)(aac_header + 3);
	body_size = ntohl(body_size); //Little Endian
	body_size = body_size << 6;
	body_size = body_size >> 19;

	true_size = fread(buf, 1, body_size - 7, fp_AAC);
	//printf("true_size:%d body_size:%d\n",true_size,body_size);
	return true_size;
}

void *fill_thread(void *arg)  
{  
	NaluUnit naluUnit;
	unsigned char tmp[512*1024] = {0};
    while(1)
    {
		int true_size = read_h264(tmp, 512*1024);
		ringput(tmp,true_size,1);
		memset(tmp, 0x0, 512*1024);
		int size = ringsize();
		if(size > 3)
		{
			usleep(600*1000);
		}
		else
		{
			usleep(500*1000);
		}
	}
	
    return ((void *)0);  
}

int main(int argc, char* argv[])
{
	int ret = 0;
	if(argc < 4)
	{
		fprintf(stderr,"video:cuc_ieschool.h264 audio:test.aac url:rtmp://192.168.100.28/live/livestream default\n");
		//fp_h264 = fopen("cuc_ieschool.h264", "rb");
		//fp_h264 = fopen("bitstream.h264", "rb");
		fp_h264 = fopen("test.264", "rb");
		//fp_h264 = fopen("test10.264", "rb");

		fp_AAC = fopen("test.aac", "rb");
	//	ringmalloc(512*1024);

	//	int ret;  
	//	if((ret = pthread_create(&ntid,NULL,fill_thread,NULL))!= 0)  
	//	{  
	//		printf("can't create thread: %s\n",strerror(ret));  
	//		return -1;  
	//	}

		//��ʼ�������ӵ�������
		ret = RTMP264_Connect("rtmp://192.168.100.28/live/livestream");
		//ret = RTMP264_Connect("rtmp://192.168.103.8/live/livestream");
	}
	else
	{
		fp_h264 = fopen(argv[0], "rb");
		fp_AAC = fopen(argv[1], "rb");
		ret = RTMP264_Connect(argv[2]);
	}

	if(ret == 0)
	{
		fprintf(stderr,"RTMP264_Connect ERROR\n");
		exit(0);
	}

	//����
//	pthread_t tid;
//	if((ret = pthread_create(&tid,NULL,RTMP264_Send,read_buffer1))!= 0)  
//	{  
//		printf("can't create thread: %s\n",strerror(ret));  
//		return -1;  
//	}
	//RTMP264_Send(read_buffer0);
	//RTMPMulti_Send(read_buffer0, read_AAC);

	//RTMP264_Send(read_h264);
	//RTMPAAC_Send(read_AAC);

	RTMPMulti_Send(read_h264, read_AAC);
	//RTMPMulti_Send(read_h264, NULL);


	//�Ͽ����Ӳ��ͷ������Դ
	RTMP264_Close();
	ringfree();

	return 0;
}

