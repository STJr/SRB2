#define LOG_TAG "SRB2-main"
#include "utils/Log.h"

#include "../doomdef.h"
#include "../d_main.h"
#include "../m_argv.h"

#include "i_video.h"

#include "jni_main.h"

int srb2_main()
{
        // startup SRB2
        CONS_Printf ("Setting up SRB2 (fo' real)...");
        D_SRB2Main();
        CONS_Printf ("Entering main game loop...");
        // never return
        D_SRB2Loop();
        LOGD("Control left SRB2.  Good bye.");

        // return to OS
        return 0;
}

JNIEXPORT jint JNICALL Java_org_srb2_nativecode_Main_main
(JNIEnv * env, jobject self, jobject video) {
  jobject fbBuf;
  jfieldID fbBufField;

  // a global reference to JNI Env, so my callbacks can use it:
  jni_env = env;
  androidVideo = video;

  jclass videoClass = (*env)->FindClass(env, "org/srb2/nativecode/Video");
  if(videoClass == NULL) {
    LOGE("Could not find Video class from JNI!");
    return -1;
  }
  fbBufField = (*env)->GetFieldID(env, videoClass, "fb", "Ljava/nio/ByteBuffer;");
  fbBuf = (*env)->GetObjectField(env, video, fbBufField);
  if(fbBuf == NULL) {
    LOGE("Couldn't get Video object from JNI!");
    return -1;
  }
  videoFrameCB = (*env)->GetMethodID(env, videoClass, "gotFrame", "()V");
  if(videoFrameCB == NULL) {
    LOGE("Couldn't get method ID of Video#gotFrame() callback!");
    return -1;
  }
  android_surface = (UINT8*) (*env)->GetDirectBufferAddress(env, fbBuf);
  return srb2_main();
}
