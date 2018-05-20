LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libv8_base
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libv8_base.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libv8_snapshot
LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libv8_snapshot.a
include $(PREBUILT_STATIC_LIBRARY)

# local
include $(CLEAR_VARS)

LOCAL_CFLAGS := -std=c++11

LOCAL_STATIC_LIBRARIES := v8_base v8_snapshot

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_MODULE := hypercasino
LOCAL_SRC_FILES := main.cpp Configuration.cpp Wrappable.cpp Event.cpp V8Event.cpp
LOCAL_LDLIBS := -llog -lGLESv2 -landroid

include $(BUILD_SHARED_LIBRARY)

# $(call import-add-path,$(LOCAL_PATH))
