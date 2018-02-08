#include <jni.h>
#ifndef __JNI_LIVERTMP__
#define __JNI_LIVERTMP__

JNIEXPORT jlong JNICALL Java_com_RtmpPublish(JNIEnv * env, jobject thiz, jstring juri, jint len);

JNIEXPORT void JNICALL Java_com_RtmpClose(JNIEnv * env, jobject thiz , jlong ctx );

JNIEXPORT jboolean JNICALL Java_com_RtmpPushArrayByte(JNIEnv* env,  jobject thiz, jlong ctx , jboolean isAudio, jbyteArray byteArrayData, jint offset, jint size , jlong pts );

JNIEXPORT jboolean JNICALL Java_com_RtmpPushByteBuffer(JNIEnv* env, jobject thiz, jlong ctx , jboolean isAudio, jobject byteBufferData, jint offset, jint size  , jlong pts );

#endif
