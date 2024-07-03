/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

package com.example.android_app;

import android.os.Bundle;

public class SessionStats {

    public static final String KEY_STATS_APP_1 = "STATS_APP_1";
    public static final String KEY_STATS_APP_2 = "STATS_APP_2";
    public static final String KEY_SESSION_ID = "SESSION_ID";
    public static final String KEY_DSP_TIME_US = "DSP_TIME_US";
    public static final String KEY_CPU_TIME_US = "CPU_TIME_US";
    public static final String KEY_TEST_TIME_US = "TEST_TIME_US";
    public static final String KEY_WAITING_ON_RESOURCE_TIME_US = "WAITING_ON_RESOURCE_TIME_US";
    public static final String KEY_NUM_TASKS_INTERRUPTED = "NUM_TASKS_INTERRUPTED";
    public static final String KEY_NUM_INTERRUPTIONS = "NUM_INTERRUPTIONS";
    public static final String KEY_NUM_TASKS_ABORTED = "NUM_TASKS_ABORTED";
    public static final String KEY_NUM_TASKS_FAILED = "NUM_TASKS_FAILED";
    public static final String KEY_NUM_TASKS_COMPLETED = "NUM_TASKS_COMPLETED";
    private static final String TAG = "AA.ApplicationStats";

    private int sessionId;
    private long  dspTimeUs;
    private long cpuTimeUs;
    private long testTimeUs;
    private long waitingOnResourceTimeUs;
    private int numTasksInterrupted;
    private int numTasksAborted;
    private int numTasksFailed;
    private int numTasksCompleted;
    private int numInterruptions;

    public SessionStats(Bundle bundle) {
        sessionId=bundle.getInt(KEY_SESSION_ID);
        dspTimeUs=bundle.getLong(KEY_DSP_TIME_US);
        cpuTimeUs=bundle.getLong(KEY_CPU_TIME_US);
        testTimeUs=bundle.getLong(KEY_TEST_TIME_US);
        waitingOnResourceTimeUs=bundle.getLong(KEY_WAITING_ON_RESOURCE_TIME_US);
        numTasksInterrupted=bundle.getInt(KEY_NUM_TASKS_INTERRUPTED);
        numTasksAborted=bundle.getInt(KEY_NUM_TASKS_ABORTED);
        numTasksFailed=bundle.getInt(KEY_NUM_TASKS_FAILED);
        numTasksCompleted=bundle.getInt(KEY_NUM_TASKS_COMPLETED);
        numInterruptions=bundle.getInt(KEY_NUM_INTERRUPTIONS);
    }


    public Bundle getBundle() {
        Bundle settingsBundle = new Bundle();
        settingsBundle.putInt(KEY_SESSION_ID,sessionId);
        settingsBundle.putLong(KEY_DSP_TIME_US,dspTimeUs);
        settingsBundle.putLong(KEY_CPU_TIME_US,cpuTimeUs);
        settingsBundle.putLong(KEY_TEST_TIME_US,testTimeUs);
        settingsBundle.putLong(KEY_WAITING_ON_RESOURCE_TIME_US,waitingOnResourceTimeUs);
        settingsBundle.putInt(KEY_NUM_TASKS_INTERRUPTED,numTasksInterrupted);
        settingsBundle.putInt(KEY_NUM_TASKS_ABORTED,numTasksAborted);
        settingsBundle.putInt(KEY_NUM_TASKS_FAILED,numTasksFailed);
        settingsBundle.putInt(KEY_NUM_TASKS_COMPLETED,numTasksCompleted);
        settingsBundle.putInt(KEY_NUM_INTERRUPTIONS,numInterruptions);
        return settingsBundle;
    }

    public SessionStats(int sessionId) {
        this.sessionId=sessionId;
        dspTimeUs=0;
        cpuTimeUs=0;
        testTimeUs=0;
        waitingOnResourceTimeUs=0;
        numTasksInterrupted=0;
        numTasksAborted=0;
        numTasksFailed=0;
        numTasksCompleted=0;
        numInterruptions=0;
    }

    public int getApplicationId() {
        return sessionId;
    }

    public void setApplicationId(int sessionId) {
        this.sessionId=sessionId;
    }

    public long getDspTimeUs() {
        return dspTimeUs;
    }

    public void setDspTimeUs(long dspTimeUs) {
        this.dspTimeUs=dspTimeUs;
    }

    public long getTestTimeUs() {
        return testTimeUs;
    }

    public void setTestTimeUs(long testTimeUs) {
        this.testTimeUs=testTimeUs;
    }

    public long getCpuTimeUs() {
        return cpuTimeUs;
    }

    public void setCpuTimeUs(long cpuTimeUs) {
        this.cpuTimeUs=cpuTimeUs;
    }

    public long getWaitingOnResourceTimeUs() {
        return waitingOnResourceTimeUs;
    }

    public void setWaitingOnResourceTimeUs(long waitingOnResourceTimeUs) {
        this.waitingOnResourceTimeUs=waitingOnResourceTimeUs;
    }

    public int getNumTasksFailed() {
        return numTasksFailed;
    }

    public void setNumTasksFailed(int numTasksFailed) {
        this.numTasksFailed=numTasksFailed;
    }

    public int getNumTasksCompleted() {
        return numTasksCompleted;
    }

    public void setNumTasksCompleted(int numTasksCompleted) {
        this.numTasksCompleted=numTasksCompleted;
    }

    public int getNumTasksInterrupted() {
        return numTasksInterrupted;
    }

    public void setNumTasksInterrupted(int numTasksInterrupted) {
        this.numTasksInterrupted=numTasksInterrupted;
    }

    public int getNumTasksAborted() {
        return numTasksAborted;
    }

    public void setNumTasksAborted(int numTasksAborted) {
        this.numTasksAborted=numTasksAborted;
    }

    public float getFastrpcOverheadPerCall() {
        if (dspTimeUs==0) {
            return 0;
        }
        return (cpuTimeUs-dspTimeUs)/(float)(numTasksAborted+numTasksCompleted+numTasksFailed);
    }

    public float getAvgNumTasksCompletedPerSecond() {
        if (cpuTimeUs==0) {
            return 0;
        }
        return numTasksCompleted/(float)cpuTimeUs* 1000000;
    }

    public float getPercentDspTimeWaitingOnResource() {
        if (dspTimeUs==0) {
            return 0;
        }
        return waitingOnResourceTimeUs/(float)dspTimeUs;
    }

    public float getPercentActiveProcessing() {
        if (testTimeUs==0) {
            return 0;
        }
        return (dspTimeUs-waitingOnResourceTimeUs)/(float)testTimeUs;
    }
}
