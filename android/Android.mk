LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := user

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := SRB2
LOCAL_CERTIFICATE := media

LOCAL_REQUIRED_MODULES := libsrb2
LOCAL_JNI_SHARED_LIBRARIES := libsrb2

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
