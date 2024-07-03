/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include <jni.h>
#include <string>
#include <stddef/AEEStdErr.h>
#include "rpcmem.h"
#include "remote.h"
#include "app_run.h"
#include "logs.h"


#pragma weak remote_session_control

static int unsigned_pd_already_requested=0;

using namespace std;
class android_app {
public:
    int64 result;
    int nErr;
}; // object;


void populate_configs_c(JNIEnv *env, _jobject *sessionConfigs, android_app_configs* sessionConfigsC);

void populate_stats_java(JNIEnv *env, _jobject *session1Stats, const android_app_stats &session1StatsC);

extern "C" {

void convert_config(JNIEnv *env, jobject jConfig, android_app_configs* cConfig) {
    jclass cls = env->GetObjectClass(jConfig);
    jfieldID triggerPeriodMsId = env->GetFieldID(cls, "triggerPeriodUs", "I");
    return;
}

void set_dsp_path( JNIEnv *env, jstring libPath, jstring logPath) {
    const char *libPathChars = env->GetStringUTFChars(libPath, 0);
    const char *reportPathChars = env->GetStringUTFChars(logPath, 0);
    int maxLength=1024;
    int libPathLength = 0;
    if (libPathChars!=NULL) {
        libPathLength=strlen(libPathChars);
    }
    int reportPathLength = 0;
    if (reportPathChars!=NULL) {
        reportPathLength=strlen(reportPathChars);
    }
    if (libPathLength==0) {
        LOG( "Error: No library path present. This is a fatal error.");
        return;
    } else if (reportPathLength==0) {
        LOG( "Warning: No path to store stats reports.  Setting DSP_LIBRARY_PATH to %s",libPathChars);
        setenv("DSP_LIBRARY_PATH",libPathChars, 1);
        return;
    } else if (reportPathLength+libPathLength>maxLength) {
        LOG("Warning: Unexpected long path names for %s and %s./nSetting DSP_LIBRARY_PATH to %s",
            libPathChars,reportPathChars,libPathChars);
        setenv("DSP_LIBRARY_PATH",libPathChars, 1);
    } else {
        char *dsp_path = (char *)malloc(libPathLength+reportPathLength+2);
        memcpy(dsp_path,libPathChars, libPathLength);
        memcpy(dsp_path+libPathLength,";",1);
        memcpy(dsp_path+libPathLength+1,reportPathChars, reportPathLength+1);
        setenv("DSP_LIBRARY_PATH",dsp_path, 1);
        LOG( "DSP_LIBRARY_PATH set to %s",dsp_path);
        free(dsp_path);
    }
    env->ReleaseStringUTFChars(libPath, libPathChars);
    env->ReleaseStringUTFChars(logPath, reportPathChars);
}

void getReportFileName(JNIEnv *env, char* reportFileName, jstring reportPath) {
    const char *reportPathChar = env->GetStringUTFChars(reportPath, 0);
    char reportName[]="report.txt";
    int reportNameSize=strlen(reportName);
    if (reportPathChar) {
        strncpy(reportFileName, reportPathChar,
                strlen(reportPathChar));
        strncat(reportFileName, "/",  1);
        strncat(reportFileName, reportName, sizeof(reportFileName) - strlen(reportFileName) - 1);
    } else {
        LOG("Unable to generate report name");
        reportFileName=NULL;
    }
    env->ReleaseStringUTFChars(reportPath, reportPathChar);
}


JNIEXPORT jlong JNICALL
Java_com_example_android_1app_MainActivity_terminate(JNIEnv *env, jobject callingObject) {
    int returnedValue = AEE_SUCCESS;
    if (0 != (returnedValue =  app_terminate() )) {
        LOG("app_terminate call returned err 0x%x", returnedValue);
    }
    return returnedValue;
}

JNIEXPORT jlong JNICALL
Java_com_example_android_1app_MainActivity_launch(            JNIEnv *env, jobject callingObject,
                                                             jobject session1Configs, jobject session1Stats, jobject session2Configs, jobject session2Stats, jstring libPath, jstring reportPath) {
    int returnedValue = AEE_SUCCESS;

    android_app_configs* session1ConfigsC;
    android_app_configs* session2ConfigsC;
    android_app_stats* session1StatsC;
    android_app_stats* session2StatsC;
    char reportFileName[256]="";


    // JNI components run in the client's process's context so all requests for unsigned PD offload will
    // be issued from the same process.  However, older targets will reset the session attributes when
    // a session is closed and thus require to set unsigned PD attribute each time again
    int response;
    LOG("Requesting unsigned PD offload.");
    if((response=request_unsigned_pd())!=0) {
        LOG("WARNING: unsigned PD request failed: %0x", response);
        if (!unsigned_pd_already_requested) {
            LOG("ERROR: The first unsigned PD request should always be successful.");
            goto bail;
        } else {
            LOG("This warning will be ignored as older targets may reset attributes when a session is closed.");
        }
    } else {
        unsigned_pd_already_requested=1;
    }

    session1ConfigsC = (android_app_configs*)rpcmem_alloc(0, RPCMEM_HEAP_DEFAULT, sizeof(android_app_configs));
    session2ConfigsC = (android_app_configs*)rpcmem_alloc(0, RPCMEM_HEAP_DEFAULT, sizeof(android_app_configs));
    if ((session1ConfigsC==NULL)||(session2ConfigsC==NULL)) {
        LOG("Failed to allocate memory for app configurations");
        goto bail;
    }
    session1StatsC = (android_app_stats*)rpcmem_alloc(0, RPCMEM_HEAP_DEFAULT, sizeof(android_app_stats));
    session2StatsC = (android_app_stats*)rpcmem_alloc(0, RPCMEM_HEAP_DEFAULT, sizeof(android_app_stats));
    if ((session1StatsC==NULL)||(session2StatsC==NULL)) {
        LOG( "Failed to allocate memory for app statistics");
        goto bail;
    }

    populate_configs_c(env, session1Configs, session1ConfigsC);
    populate_configs_c(env, session2Configs, session2ConfigsC);

    getReportFileName(env, reportFileName, reportPath);
    set_dsp_path(env, libPath, reportPath);

    returnedValue = app_run(session1ConfigsC, session1StatsC, session2ConfigsC,
                            session2StatsC, reportFileName);

    if (0 != returnedValue) {
        LOG("app_run call returned err 0x%x", returnedValue);
        goto bail;
    }

    populate_stats_java(env, session1Stats, *session1StatsC);
    populate_stats_java(env, session2Stats, *session2StatsC);

    bail:
    if (session1StatsC) {
        rpcmem_free(session1StatsC);
    }
    if (session2StatsC) {
        rpcmem_free(session2StatsC);
    }
    if (session1ConfigsC) {
        rpcmem_free(session1ConfigsC);
    }
    if (session2ConfigsC) {
        rpcmem_free(session2ConfigsC);
    }
    return returnedValue;
}
}

/* A word of caution: None of the variable names below are checked at compile time. A mismatch between the strings and the
 * actual variable name will cause an exception a runtime. */

void populate_stats_java(JNIEnv *env, _jobject *session1Stats, const android_app_stats &session1StatsC) {
    jclass session1StatsClass = env->GetObjectClass(session1Stats);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "sessionId", "I"), session1StatsC.session_id);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "numTasksCompleted", "I"), session1StatsC.num_tasks_completed);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "numTasksAborted", "I"), session1StatsC.num_tasks_aborted);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "numTasksFailed", "I"), session1StatsC.num_tasks_failed);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "numTasksInterrupted", "I"), session1StatsC.num_tasks_interrupted);
    env->SetIntField(session1Stats, env->GetFieldID(session1StatsClass, "numInterruptions", "I"), session1StatsC.num_interruptions);
    env->SetLongField(session1Stats, env->GetFieldID(session1StatsClass, "dspTimeUs", "J"), (  int64_t ) session1StatsC.dsp_time_us);
    env->SetLongField(session1Stats, env->GetFieldID(session1StatsClass, "waitingOnResourceTimeUs", "J"), (  int64_t ) session1StatsC.waiting_on_resource_time_us);
    env->SetLongField(session1Stats, env->GetFieldID(session1StatsClass, "cpuTimeUs", "J"), (  int64_t ) session1StatsC.cpu_time_us);
    env->SetLongField(session1Stats, env->GetFieldID(session1StatsClass, "testTimeUs", "J"), (  int64_t ) session1StatsC.test_time_us);
}

void populate_configs_c(JNIEnv *env, _jobject *appConfigs, android_app_configs* appConfigsC) {
    jclass cls = env->GetObjectClass(appConfigs);
    appConfigsC->session_id = env->GetIntField(appConfigs, env->GetFieldID(cls, "sessionId", "I"));

    appConfigsC->vtcm_size_mb = env->GetIntField(appConfigs, env->GetFieldID(cls, "vtcmSizeMB", "I"));
    appConfigsC->num_workers = env->GetIntField(appConfigs, env->GetFieldID(cls, "numWorkers", "I"));
    appConfigsC->mega_cycles_per_task = env->GetIntField(appConfigs, env->GetFieldID(cls, "megaCyclesPerTask", "I"));
    appConfigsC->is_serialized = env->GetBooleanField(appConfigs, env->GetFieldID(cls, "isSerialized", "Z"));
    appConfigsC->is_asynchronous = env->GetBooleanField(appConfigs, env->GetFieldID(cls, "isAsynchronous", "Z"));
    appConfigsC->trigger_period_ms = env->GetIntField(appConfigs, env->GetFieldID(cls, "triggerPeriodMs", "I"));
    jobject triggerObject = env->GetObjectField(appConfigs, env->GetFieldID(cls, "taskTrigger","Lcom/example/android_app/SessionConfigs$TaskTrigger;"));
    appConfigsC->task_trigger = (env)->CallIntMethod(triggerObject, (env)->GetMethodID((env)->FindClass("com/example/android_app/SessionConfigs$TaskTrigger"), "getValue", "()I"));
    appConfigsC->thread_priority = env->GetIntField(appConfigs, env->GetFieldID(cls, "threadPriority", "I"));
    appConfigsC->num_tasks = env->GetIntField(appConfigs, env->GetFieldID(cls, "numTasks", "I"));
    appConfigsC->is_off = env->GetBooleanField(appConfigs, env->GetFieldID(cls, "isOff", "Z"));

    // Some additional parameters not configurable from the UI (but still configurable programmatically)
    appConfigsC->array_width = 8;
    appConfigsC->array_height = 8;
    appConfigsC->resource_acquire_timeout_us = 10000;
    appConfigsC->verbose_mask = 0x1f;
}
