/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

package com.example.android_app;

import android.os.Bundle;

public class SessionConfigs {

    public enum TaskTrigger {
        QUEUED, TIMED, CAMERA;

        private final int value;
        private TaskTrigger() {
            this.value = ordinal();
        }

        public int getValue() {
            return value;
        }

        public static TaskTrigger fromValue(int value)
                throws IllegalArgumentException {
            try {
                return TaskTrigger.values()[value];
            } catch(ArrayIndexOutOfBoundsException e) {
                throw new IllegalArgumentException("Unknown enum value :"+ value);
            }
        }
    }

    public static final String KEY_CONFIGS= "CONFIGS";
    public static final String KEY_SESSION_ID = "SESSION_ID";
    public static final String KEY_TASK_TRIGGER= "TASK_TRIGGER";
    public static final String KEY_TRIGGER_PERIOD_MS= "TRIGGER_PERIOD_MS";
    public static final String KEY_VTCM_SIZE_MB= "VTCM_SIZE_MB";
    public static final String KEY_NUM_WORKERS= "NUM_WORKERS";
    public static final String KEY_MEGA_CYCLES_PER_TASK= "MEGA_CYCLES_PER_TASK";
    public static final String KEY_NUM_TASKS = "NUM_TASKS";
    public static final String KEY_IS_SERIALIZED = "IS_SERIALIZED";
    public static final String KEY_IS_NEVER_ENDING= "IS_NEVER_ENDING";
    public static final String KEY_THREAD_PRIORITY= "THREAD_PRIORITY";
    public static final String KEY_IS_ASYNCHRONOUS= "IS_ASYNCHRONOUS";
    public static final String KEY_IS_OFF= "IS_OFF";
    private static final String TAG = "AA.SessionConfigs";

    private int sessionId;
    private TaskTrigger taskTrigger;
    private int triggerPeriodMs;
    private int vtcmSizeMB;
    private int numWorkers;
    private int megaCyclesPerTask;
    private boolean isSerialized;
    private int numTasks;
    private boolean isNeverEnding;
    private int threadPriority;
    private boolean isAsynchronous;
    private boolean isOff;

    public SessionConfigs(int sessionId, TaskTrigger taskTrigger, int vtcmSizeMB, int threadPriority) {
        this.sessionId=sessionId;
        this.taskTrigger=taskTrigger;
        this.triggerPeriodMs=30;
        this.vtcmSizeMB=vtcmSizeMB;
        this.numWorkers=4;
        this.megaCyclesPerTask=15;
        this.isSerialized=true;
        this.numTasks=1;
        this.isNeverEnding=true;
        this.threadPriority=threadPriority;
        this.isAsynchronous=false;
        this.isOff=false;
    }

    public SessionConfigs(Bundle bundle) {
        this.sessionId=bundle.getInt(KEY_SESSION_ID);
        this.taskTrigger=TaskTrigger.fromValue(bundle.getInt(KEY_TASK_TRIGGER));
        this.triggerPeriodMs=bundle.getInt(KEY_TRIGGER_PERIOD_MS);
        this.vtcmSizeMB=bundle.getInt(KEY_VTCM_SIZE_MB);
        this.numWorkers=bundle.getInt(KEY_NUM_WORKERS);
        this.megaCyclesPerTask=bundle.getInt(KEY_MEGA_CYCLES_PER_TASK);
        this.numTasks=bundle.getInt(KEY_NUM_TASKS);
        this.isSerialized=bundle.getBoolean(KEY_IS_SERIALIZED);
        this.isNeverEnding=bundle.getBoolean(KEY_IS_NEVER_ENDING);
        this.threadPriority=bundle.getInt(KEY_THREAD_PRIORITY);
        this.isAsynchronous=bundle.getBoolean(KEY_IS_ASYNCHRONOUS);
        this.isOff=bundle.getBoolean(KEY_IS_OFF);
    }

    public Bundle getBundle() {
        Bundle configsBundle = new Bundle();
        configsBundle.putInt(KEY_SESSION_ID, sessionId);
        configsBundle.putInt(KEY_TASK_TRIGGER, taskTrigger.getValue());
        configsBundle.putInt(KEY_TRIGGER_PERIOD_MS, triggerPeriodMs);
        configsBundle.putInt(KEY_VTCM_SIZE_MB, vtcmSizeMB);
        configsBundle.putInt(KEY_NUM_WORKERS, numWorkers);
        configsBundle.putInt(KEY_MEGA_CYCLES_PER_TASK, megaCyclesPerTask);
        configsBundle.putInt(KEY_NUM_TASKS,numTasks);
        configsBundle.putBoolean(KEY_IS_SERIALIZED,isSerialized);
        configsBundle.putBoolean(KEY_IS_NEVER_ENDING, isNeverEnding);
        configsBundle.putInt(KEY_THREAD_PRIORITY, threadPriority);
        configsBundle.putBoolean(KEY_IS_ASYNCHRONOUS, isAsynchronous);
        configsBundle.putBoolean(KEY_IS_OFF, isOff);
        return configsBundle;
    }

    public boolean isAsynchronous() {
        return isAsynchronous;
    }
    public void setAsynchronous(boolean asynchronous) {
        isAsynchronous=asynchronous;
    }
    public SessionConfigs setNewPriority(int priority) {
        this.threadPriority=priority;
        return this;
    }
    public SessionConfigs setNewTaskTrigger(TaskTrigger taskTrigger) {
        this.taskTrigger=taskTrigger;
        return this;
    }
    public SessionConfigs setNewTriggerPeriodUs(int triggerPeriodUs) {
        this.triggerPeriodMs=triggerPeriodUs;
        return this;
    }
    public SessionConfigs setNewVtcmSizeMB(int vtcmSizeMB) {
        this.vtcmSizeMB=vtcmSizeMB;
        return this;
    }
    public SessionConfigs setNewNumWorkers(int numWorkers) {
        this.numWorkers=numWorkers;
        return this;
    }
    public SessionConfigs setNewMegaCyclesPerTask(int megaCyclesPerTask) {
        this.megaCyclesPerTask=megaCyclesPerTask;
        return this;
    }
    public SessionConfigs setNewSerialized(boolean isSerialized) {
        this.isSerialized=isSerialized;
        return this;
    }
    public SessionConfigs setNewNumTasks(int numTasks) {
        this.numTasks=numTasks;
        return this;
    }

    public boolean isOff() {
        return isOff;
    }

    public void setOff(boolean off) {
        isOff=off;
    }

    public int getThreadPriority() {
        return threadPriority;
    }

    public void setThreadPriority(int threadPriority) {
        this.threadPriority=threadPriority;
    }

    public boolean isNeverEnding() {
        return isNeverEnding;
    }

    public void setNeverEnding(boolean neverEnding) {
        isNeverEnding=neverEnding;
    }

    public boolean isSerialized() {
        return isSerialized;
    }

    public void setSerialized(boolean serialized) {
        isSerialized=serialized;
    }

    public int getNumTasks() {
        return numTasks;
    }

    public void setNumTasks(int numTasks) {
        this.numTasks=numTasks;
    }

    public int getSessionId() {
        return sessionId;
    }

    public void setSessionId(int sessionId) {
        this.sessionId=sessionId;
    }

    public TaskTrigger getTaskTrigger() {
        return taskTrigger;
    }

    public void setTaskTrigger(TaskTrigger taskTrigger) {
        this.taskTrigger=taskTrigger;
    }

    public int getTriggerPeriodMs() {
        return triggerPeriodMs;
    }

    public void setTriggerPeriodMs(int triggerPeriodMs) {
        this.triggerPeriodMs=triggerPeriodMs;
    }

    public int getVtcmSizeMB() {
        return vtcmSizeMB;
    }

    public void setVtcmSizeMB(int vtcmSizeMB) {
        this.vtcmSizeMB=vtcmSizeMB;
    }

    public int getNumWorkers() {
        return numWorkers;
    }

    public void setNumWorkers(int numWorkers) {
        this.numWorkers=numWorkers;
    }

    public int getMegaCyclesPerTask() { return megaCyclesPerTask; }

    public void setMegaCyclesPerTask(int megaCyclesPerTask) {
        this.megaCyclesPerTask=megaCyclesPerTask;
    }

}
