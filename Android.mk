# Build both the native piece of SRB2 for Android, and the Java frontend.
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(call all-makefiles-under,$(LOCAL_PATH))
