#ifndef _SRB2_ANDROID_VIDEO_
#define _SRB2_ANDROID_VIDEO_

#include <jni.h>

UINT8 *android_surface;

JNIEnv* jni_env;
jobject androidVideo;
jmethodID videoFrameCB;

#endif
