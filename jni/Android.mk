LOCAL_PATH := $(call my-dir)  
  
include $(CLEAR_VARS)

LOCAL_MODULE := hideroot
LOCAL_SRC_FILES := hideroot.c utils/vector.c
  
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog  

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)

LOCAL_MODULE := demo
LOCAL_SRC_FILES := demo.c
  
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog  

include $(BUILD_EXECUTABLE)