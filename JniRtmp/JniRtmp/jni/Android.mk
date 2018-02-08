LOCAL_PATH:= $(call my-dir)
PROJECT_LOCAL_PATH:=$(LOCAL_PATH)


#################################################################################################################
include $(CLEAR_VARS)
LOCAL_PATH:=$(PROJECT_LOCAL_PATH)

#LOCAL_PREBUILT_LIBS   		:= prebuilt/librtmp.a
#LOCAL_STATIC_LIBRARIES     	:= librtmp
#LOCAL_MULTILIB := 32
#include $(BUILD_MULTI_PREBUILT)

include $(LOCAL_PATH)/librtmp/Android.mk

#include $(LOCAL_PATH)/inc/config.h
#LOCAL_MODULE_TAGS 			:= optional
#LOCAL_ARM_MODE := arm

#################################################################################################################
include $(CLEAR_VARS)
LOCAL_PATH:=$(PROJECT_LOCAL_PATH)

LOCAL_MODULE    := liblivertmp
LOCAL_SRC_FILES := src/Livertmp.cpp \
					src/Jni_load.cpp \
					src/Jni_livertmp.cpp \
					flvparse.cpp \
					rtmp_list.c \
					rtmp_poll.cpp \
					rtmp_push.cpp 

LOCAL_C_INCLUDES += \
   $(LOCAL_PATH)/include  \
   $(LOCAL_PATH)/librtmp \
   $(LOCAL_PATH)

LOCAL_MULTILIB := 32

LOCAL_STATIC_LIBRARIES := librtmp
LOCAL_LDLIBS := -llog -landroid -ljnigraphics
LOCAL_SHARED_LIBRARIES := liblog libandroid libjnigraphics

include $(BUILD_SHARED_LIBRARY)

#LOCAL_SDK_VERSION   := 9
#LOCAL_ARM_MODE := arm
#LOCAL_SHARED_LIBRARIES :=
#LOCAL_CFLAGS :=
#LOCAL_CPPFLAGS += $(JNI_CFLAGS)
#LOCAL_LDFLAGS :=
