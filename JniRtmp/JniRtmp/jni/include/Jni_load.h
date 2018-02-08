#ifndef __JNI_LOAD___
#define __JNI_LOAD___

#ifdef __cplusplus
extern "C" {
#endif
#include <jni.h>

//global java vm and env
extern JavaVM *g_JVM;

#ifdef __cplusplus
	#define STREAMER_ATTACH_JVM(jni_env) \
	JNIEnv *jenv;\
	int status = g_JVM->GetEnv((void **)&jenv, JNI_VERSION_1_4); \
	jint result = g_JVM->AttachCurrentThread(&jni_env,NULL);

	#define STREAMER_DETACH_JVM(jni_env) if( status == JNI_EDETACHED ){ g_JVM->DetachCurrentThread(); }
#else
	#define STREAMER_ATTACH_JVM(jni_env) \
	JNIEnv *jenv;\
	int status = (*g_JVM)->GetEnv(android_jvm, (&jenv), JNI_VERSION_1_4); \
	jint result = (*g_JVM)->AttachCurrentThread(g_JVM, &jni_env,NULL);

	#define STREAMER_DETACH_JVM(jni_env) if(status == JNI_EDETACHED ){ (*g_jvm)->DetachCurrentThread(android_jvm); }
#endif

typedef enum _JAVA_CLASS_TYPE_
{
	JAVA_CLASS_STREAMER = 0,
	JAVA_CLASS_INTEGER,
	JAVA_CLASS_MAX,
}JAVA_CLASS_TYPE;

extern const char* g_java_class_name[];
extern jobject g_java_class_obj[JAVA_CLASS_MAX];
extern jmethodID g_java_class_construct[JAVA_CLASS_MAX];

#define JNI_GET_CLASS_NAME(type) 					g_java_class_name[(type)]
#define JNI_GET_CLASS_OBJ(type)						g_java_class_obj[(type)]
#define JNI_GET_CLASS_CONSTRUCT_METHODE(type)		g_java_class_construct[(type)]


//AfPalmchat static method
typedef enum _JAVA_STATIC_METHOD_
{
   JAVA_STATIC_METHOD_MSG_HANDLE = 0
   ,JAVA_STATIC_METHOD_RESULT
   ,JAVA_STATIC_METHOD_MAX
}JAVA_STATIC_METHOD;

extern jmethodID g_static_method[JAVA_STATIC_METHOD_MAX];
#define JNI_GET_STATIC_METHOD(type)					g_static_method[(type)]
#define JNI_SET_STATIC_METHOD(type, method)         g_static_method[(type)] = (method)



//
#define JNI_GET_FIELD_OBJ_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/Object;")
#define JNI_GET_FIELD_OBJ_ID_NAME(name)		env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/Object;")

#define JNI_GET_FIELD_STR_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/String;")
#define JNI_GET_FIELD_STR_ID_NAME(name)		env->GetFieldID(jclazz, (const char*)name,  "Ljava/lang/String;")

#define JNI_GET_FIELD_INT_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "I")
#define JNI_GET_FIELD_INT_ID_NAME(name)	    env->GetFieldID(jclazz, (const char*)name,  "I")

#define JNI_GET_FIELD_BOOLEAN_ID(jclazz, name)  	env->GetFieldID(jclazz, (const char*)name,  "Z")
#define JNI_GET_FIELD_BOOLEAN_ID_NAME(name)  		env->GetFieldID(jclazz, (const char*)name,  "Z")

#define JNI_GET_FIELD_LONG_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "J")
#define JNI_GET_FIELD_LONG_ID_NAME(name)	env->GetFieldID(jclazz, (const char*)name,  "J")

#define JNI_GET_FIELD_DOUBLE_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "D")
#define JNI_GET_FIELD_DOUBLE_ID_NAME(name)	env->GetFieldID(jclazz, (const char*)name,  "D")

#define JNI_GET_FIELD_BYTE_ID(jclazz, name)	env->GetFieldID(jclazz, (const char*)name,  "B")
#define JNI_GET_FIELD_BYTE_ID_NAME(name)	env->GetFieldID(jclazz, (const char*)name,  "B")

//
#define JNI_GET_METHOD_ID(name, sig)		env->GetMethodID(jclazz, name, sig)


//get value
#define JNI_GET_OBJ_FIELD(obj, methodId)	env->GetObjectField(obj, methodId)
#define JNI_GET_OBJ_FIELD_ID(methodId)		env->GetObjectField(obj, methodId)

#define JNI_GET_INT_FIELD(obj, methodId)	env->GetIntField(obj, methodId)
#define JNI_GET_INT_FIELD_ID(methodId)	    env->GetIntField(obj, methodId)

#define JNI_GET_LONG_FILED(obj, methodId)	env->GetLongField(obj, methodId)
#define JNI_GET_LONG_FILED_ID(methodId)	    env->GetLongField(obj, methodId)

#define JNI_GET_DOUBLE_FILED(obj, methodId)	env->GetDoubleField(obj, methodId)
#define JNI_GET_DOUBLE_FILED_ID(methodId)	env->GetDoubleField(obj, methodId)

#define JNI_GET_BYTE_FILED(obj, methodId)	env->GetByteField(obj, methodId)
#define JNI_GET_BYTE_FILED_ID(methodId)		env->GetByteField(obj, methodId)

#define JNI_GET_BOOLEAN_FIELD(obj, methodId)	env->GetBooleanField(obj, methodId)
#define JNI_GET_BOOLEAN_FIELD_ID(methodId)	    env->GetBooleanField(obj, methodId)

#define JNI_GET_UTF_STR(str, str_obj, type)		if(NULL != str_obj){\
												str = (type)env->GetStringUTFChars(str_obj, NULL);\
												}else{\
												str = (type)NULL;\
												}
#define JNI_GET_UTF_CHAR(str, str_obj)  		JNI_GET_UTF_STR(str, str_obj, char*);

#define JNI_RELEASE_STR_STR(str, str_obj)		if( NULL != str_obj && NULL != str){\
													env->ReleaseStringUTFChars(str_obj, (const char*)str);\
												}

#define JNI_GET_BYTE_ARRAY(b, array)			if( NULL != array){\
													b = (INT8S*)env->GetByteArrayElements(array, (jboolean*)NULL);\
													}else{\
														b = NULL;\
													}
#define JNI_RELEASE_BYTE_ARRAY(b, array)		if( NULL != array){\
													env->ReleaseByteArrayElements(array, (jbyte*)b, JNI_ABORT);\
												}

#define JNI_DELETE_LOCAL_REF(obj)			if( NULL != obj){\
												env->DeleteLocalRef(obj);\
											}

#ifdef __cplusplus
}
#endif

#endif//__JNI_LOAD___