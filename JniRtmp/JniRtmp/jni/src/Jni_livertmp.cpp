#include "Jni_livertmp.h"
#include "Jni_load.h"
#include "utils.h"
#include "dataType.h"
#include "Livertmp.h"


#define LOG_TAG "jni_livertmp"
#include "fast_log.h"

JNIEXPORT jlong JNICALL Java_com_RtmpPublish(JNIEnv * env, jobject thiz, jstring juri, jint len)
{
	LiveRtmp *rtmpLive = NULL;
    char *uri = NULL;
	rtmpLive = LiveRtmp::createLiveRtmp(); 
	if(0 == rtmpLive)
	{
		return 0;
	}
	JNI_GET_UTF_CHAR(uri, juri);
	int ret = rtmpLive->publishRtmp(uri, len);	
	ALOGD("publish %p %s %d " , rtmpLive , uri , ret );// 成功1，失败0
	JNI_RELEASE_STR_STR(uri, juri);
	
	if( ret != 1){
		ALOGE("push rtmp error");
		delete rtmpLive;
	    rtmpLive = NULL;
		return  0 ;
	}else{
		ALOGD("publish rtmp ok");
	}
	return (jlong)rtmpLive;
}

JNIEXPORT void JNICALL Java_com_RtmpClose(JNIEnv * env, jobject thiz , jlong ctx )
{
	LiveRtmp * rtmpLive = (LiveRtmp *)ctx ;
	if(NULL != rtmpLive)
	{
		ALOGD("close");
		rtmpLive->closeRtmp();
		delete rtmpLive;
	    rtmpLive = NULL;
	}else{
		ALOGE("close again! ");
	}
	return;
}



JNIEXPORT jboolean JNICALL Java_com_RtmpPushArrayByte(JNIEnv* env,  jobject thiz, jlong ctx , jboolean isAudio, jbyteArray byteArrayData, jint offset, jint size , jlong pts_us )
{
	LiveRtmp * rtmpLive = (LiveRtmp *)ctx ;
	if(NULL == rtmpLive)
	{
		ALOGE("push array byte with Context == null ");
		return JNI_FALSE ; 
	}
	signed char* buffer = NULL;
	JNI_GET_BYTE_ARRAY(buffer, byteArrayData);
	ALOGD("push ByteArray isAudio %d  buf %p offset %d size %d\n" , isAudio, buffer , offset , size  );
	bool ret = false ;
	if( isAudio == JNI_FALSE ){
		ret = rtmpLive->pushVideo(buffer + offset , size , pts_us);
	}else{
		ret = rtmpLive->pushAudio(buffer + offset , size , pts_us);
	}
	ALOGD("push ByteArray ret = %d " , ret );
	JNI_RELEASE_BYTE_ARRAY(buffer, byteArrayData);
	
	return (ret == true ) ? JNI_TRUE  : JNI_FALSE  ;
}

static struct timeval last_audio_tv = {0 , 0};
JNIEXPORT jboolean JNICALL Java_com_RtmpPushByteBuffer(JNIEnv* env, jobject thiz, jlong ctx , jboolean isAudio, jobject byteBufferData, jint offset, jint size , jlong pts_us )
{
 	LiveRtmp * rtmpLive = (LiveRtmp *)ctx ;
	if(NULL == rtmpLive)
	{
		ALOGE("push array byte with Context == null ");
		return JNI_FALSE ; 
	}
	
	signed char* buffer = reinterpret_cast<jbyte*>(env->GetDirectBufferAddress(byteBufferData));
	ALOGD("push ByteBuffer isAudio %d  buf %p offset %d size %d\n" , isAudio, buffer , offset , size  );
	bool ret = false ;
 	if( isAudio == JNI_FALSE ){
		ret = rtmpLive->pushVideo(buffer + offset, size , pts_us);
	}else{
		ret = rtmpLive->pushAudio(buffer + offset, size , pts_us);
	}
	ALOGD("push ByteBuffer ret = %d " , ret );

	return (ret == true ) ? JNI_TRUE  : JNI_FALSE  ;
}

