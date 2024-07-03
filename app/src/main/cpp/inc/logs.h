/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#ifndef logs_H
#define logs_H

#include <android/log.h>
#include <stdio.h>

// Notes about logs
// printf from apk will show in command line when executing test exe but not when running app
// logcat allows to capture DSP and APPs messages in one window
// adb logcat | grep -E "AA|adsprpc :"
// piping to grep is needed because logcat -E isn't supported on all versions of logcat
// the " :" is needed to filter out all other messages just containing adsprpc in the string
// adb logcat -s AA adsprpc also works but misses messages where the tag is AA<something> such
// as AA.MainActivity
#define TAG "AA"
#define LOG(...) {__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);printf(__VA_ARGS__);printf("\n");}
#define LOG_AND_SAVE(...) {\
    __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);\
    printf(__VA_ARGS__);\
    printf("\n");\
    if (file_pointer) { \
       fprintf(file_pointer,__VA_ARGS__);\
       fprintf(file_pointer,"\n");\
    }\
}

#endif // logs_H