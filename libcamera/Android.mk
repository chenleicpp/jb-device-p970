LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE         := camera.p970
LOCAL_SRC_FILES      := cameraHAL.cpp
LOCAL_C_INCLUDES := $(TOP)/frameworks/base/include
LOCAL_PRELINK_MODULE := false

LOCAL_SHARED_LIBRARIES += \
    liblog \
    libutils \
    libbinder \
    libcutils \
    libmedia \
    libhardware \
    libcamera_client \
    libcamera \
    libui

LOCAL_SHARED_LIBRARIES += libdl

include $(BUILD_SHARED_LIBRARY)
