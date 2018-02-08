#include <rtmp.h>
#include "stdio.h"
#include <unistd.h>
#include <stdlib.h>
#include "RtmpPusher.h"
#include "H264Handle.h"
#include "BufferManager.h"

int main(int argc, char* argv[])
{
	CBufferManager* VbuffMgr = new CBufferManager();
	VbuffMgr->Init(5 * 1024 * 1024);
	
	CRtmpPusher* rtmpPuser = new CRtmpPusher(VbuffMgr);

	int ret = rtmpPuser->Prepare("rtmp://192.168.1.107/live/A99762001001001");
	printf("Prepare ret:%d\n",ret);
	ret = rtmpPuser->Start();
	printf("Start ret:%d\n",ret);

	int length = 0;
	uint64_t pts = 0;
	CH264Handle* h264Handle = new CH264Handle("test.264");
	//CH264Handle* h264Handle = new CH264Handle("10_00.h264");
	unsigned char *frame = (unsigned char*)malloc(MAX_NALU_SIZE);
	while((length = h264Handle->GetAnnexbFrame(frame, MAX_NALU_SIZE)) > 0)
	{
		if(VbuffMgr != NULL)
		{
			VbuffMgr->Write(frame, length, pts);
			int type = frame[4] & 0x1f;
			fprintf(stderr, "h264Handle->GetAnnexbFrame length:%d type:%d\n", length, type);
			if(type == 5 || type == 1)
			{
				pts += 41;
				usleep(40000);
				//pts += 33;
				//usleep(33000);
			}
		}
	}
	
	free(frame);
	delete h264Handle;

	rtmpPuser->Stop();
	delete rtmpPuser;

	
	if (VbuffMgr != NULL)
	{
		delete VbuffMgr;
		VbuffMgr = NULL;
	}
	
	return 0;
}
