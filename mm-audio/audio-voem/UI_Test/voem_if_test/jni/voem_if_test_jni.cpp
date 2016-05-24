/*============================================================================
  @file ppt_voice_native.cpp
  This module contains the implementation of the JNI of voice post processing
  test.

  Copyright (c) 2010-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
*//*========================================================================*/

#include "JNIHelp.h"
#include <stdio.h>
#include <dlfcn.h>
#include <jni.h>
#include <utils/Log.h>
#include "common_log.h"

#include "android_runtime/AndroidRuntime.h"
#include "utils/misc.h"
//#define LOG_TAG "PostprocessingtestVoice"

extern "C"
{
 int  command( int feature,int enable);
 void oncrpc_start();
 void oncrpc_stop();
}



namespace android
{
int success;
int enable=1;
int disable=0;

static void  oncrpc_start_cpp(JNIEnv * env, jclass Class)
{
	oncrpc_start();
}

static int  command_cpp(JNIEnv *env, jclass Class, jint feature, jboolean flag)
{
	if(!flag)
		success=command(feature,enable);
	else
		success=command(feature,disable);
return success;
}

static void oncrpc_stop_cpp(JNIEnv *env, jclass Class)

{
oncrpc_stop();
}

static JNINativeMethod sMethods[]=
{
 /* name, signature, funcPtr */
	{"oncrpc_start","()V",(void*)oncrpc_start_cpp},
	{"command","(IZ)I",(void*)command_cpp},
	{"oncrpc_stop","()V",(void*)oncrpc_stop_cpp}
};
int register_VoemTest(JNIEnv* env)
{
   return jniRegisterNativeMethods(env, "com/android/VoemTest/VoemTest", sMethods, NELEM(sMethods));
}


}

