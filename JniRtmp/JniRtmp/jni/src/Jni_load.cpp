#include "Jni_load.h"
#include "utils.h"
#include "Jni_livertmp.h"
#include <stdio.h>


#define LOG_TAG "jni_livertmp"
#include "fast_log.h"


JavaVM *g_JVM = NULL;

#define JAVA_CLASS_NAME "com/digua/streaming/rtmp/LiveRtmp"

static JNINativeMethod g_method_table[] = {
    //name,signature,fnPtr
	{ "native_rtmpPublish", 		"(Ljava/lang/String;I)J", 			(void*)Java_com_RtmpPublish },
	{ "native_rtmpPushArrayByte",	"(JZ[BIIJ)Z", 						(void*)Java_com_RtmpPushArrayByte},
	{ "native_rtmpPushByteBuffer", 	"(JZLjava/nio/ByteBuffer;IIJ)Z", 	(void*)Java_com_RtmpPushByteBuffer },
	{ "native_rtmpClose", 			"(J)Z", 							(void*)Java_com_RtmpClose }
};


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;
	jclass clazz;
	jint result = -1;
	jint i;

	ALOGD("JNI_OnLoad: entry...\n");
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
	{
		ALOGE("ERROR: GetEnv failed\n");
		goto error;
	}

	clazz = env->FindClass(JAVA_CLASS_NAME) ; 
	if( clazz == NULL ) 
	{
		ALOGE("ERROR: FindClass %s failed\n" , JAVA_CLASS_NAME);
		goto error;
	}

	if (env->RegisterNatives(clazz, g_method_table, sizeof(g_method_table) / sizeof(JNINativeMethod)) < 0)
	{
		ALOGE("RegisterNatives failed");
        goto error;
    }

	result = JNI_VERSION_1_4;
	g_JVM = vm;

	ALOGD("JNI_OnLoad: entry out...\n");

error:
	return result;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved)
{
	JNIEnv *env;
	jclass k;
	jint r;
	jobject obj;
	
	ALOGD("JNI_OnUnload: entry...\n");

	r = vm->GetEnv((void **) &env, JNI_VERSION_1_4 );
	k = env->FindClass ( JAVA_CLASS_NAME );
	env->UnregisterNatives(k);
	
	ALOGD("JNI_OnUnload: entry out...\n");
}
