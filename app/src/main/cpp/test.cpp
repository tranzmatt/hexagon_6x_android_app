/*==============================================================================
  Copyright (c) 2024 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include "rpcmem.h"
#include "remote.h"
#include "app_run.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "AEEStdErr.h"
#include "verify.h"
#include "../../../../dsp/android_app_imp_defs.h"
#include "logs.h"
#include "android_app_defs.h"
#include "dsp_capabilities_utils.h"
#include "os_defines.h"
#include "gtest/gtest.h"
#define DEFAULT_VTCM_SIZE_MB 256
#define DEFAULT_NUM_WORKERS 4
#define DEFAULT_MEGA_CYCLES_PER_TASK 10
#define DEFAULT_IS_SERIALIZED 1
#define DEFAULT_TRIGGER_PERIOD_MS 5
#define DEFAULT_THREAD_PRIORITY 128
#define DEFAULT_NUM_TASKS 1
#define DEFAULT_IS_ASYNCHRONOUS 0
#define DEFAULT_WIDTH 8
#define DEFAULT_HEIGHT 8
// TODO: Replace with Java TaskTrigger.TIMED
#define DEFAULT_TRIGGER_TYPE 1
#define DEFAULT_RESOURCE_ACQUIRE_TIMEOUT_US 100000UL
#define DEFAULT_VERBOSE_MASK 0
// TODO: Do not hardcode
#define CLOCK_SPEED_MHZ 1400
#define TEST_SUITE_NAME run_on_dsp
#define TEST_SUITE_NAME_ASYNC async_tests
#define THRESHOLD_MULTIPLIER 4
#define VAL(str) #str
#define TOSTRING(str) VAL(str)
// Innocuous to have globals for a test executable
static android_app_configs c0;
static android_app_configs c1;
static android_app_stats s0;
static android_app_stats s1;
static int num_test=0;
static int test_to_run = -1;
static int num_hvx_units = 0;

int new_test(char const* title) {
    printf("*** Test %d: %s ***\n",test_to_run,title);
    android_app_configs* configs[] = {&c0,&c1};
    for (int i=0;i<2;i++) {
        configs[i]->session_id=i+1;
        configs[i]->vtcm_size_mb=DEFAULT_VTCM_SIZE_MB;
        configs[i]->num_workers=DEFAULT_NUM_WORKERS;
        configs[i]->mega_cycles_per_task=(num_hvx_units==2?DEFAULT_MEGA_CYCLES_PER_TASK/2:DEFAULT_MEGA_CYCLES_PER_TASK);
        configs[i]->is_serialized=DEFAULT_IS_SERIALIZED;
        configs[i]->trigger_period_ms=DEFAULT_TRIGGER_PERIOD_MS;
        configs[i]->thread_priority =DEFAULT_THREAD_PRIORITY;
        configs[i]->num_tasks=DEFAULT_NUM_TASKS;
        configs[i]->is_asynchronous=DEFAULT_IS_ASYNCHRONOUS;
        configs[i]->array_width=DEFAULT_WIDTH;
        configs[i]->array_height=DEFAULT_HEIGHT;
        configs[i]->is_off=0;
        configs[i]->resource_acquire_timeout_us=DEFAULT_RESOURCE_ACQUIRE_TIMEOUT_US;
        configs[i]->task_trigger=DEFAULT_TRIGGER_TYPE;
        configs[i]->verbose_mask=DEFAULT_VERBOSE_MASK;
    }
    return 1;
}
int num_failed_tests = 0;
int mega_cycles_per_task = 0;
const char* report_path="./report_unit_tests.txt";
TEST(TEST_SUITE_NAME,app_run_test1) {
            EXPECT_EQ(1,new_test("Two threads, same prio, serialized, log enabled"));
            c0.verbose_mask = c1.verbose_mask = 0x1f;
            app_run(&c0,&s0,&c1,&s1,report_path);
            EXPECT_EQ(2*DEFAULT_NUM_TASKS,s0.num_tasks_completed+s1.num_tasks_completed)<< "Both apps expected to succeed";
            EXPECT_EQ(0,s0.num_tasks_aborted+s1.num_tasks_aborted) << "No abortion expected";
            EXPECT_EQ(0,s0.num_failed_tests) << "DSP Output should be correct";
            EXPECT_EQ(0,s1.num_failed_tests) << "DSP Output should be correct";
        }
TEST(TEST_SUITE_NAME,app_run_test2) {
            EXPECT_EQ(1,new_test("Two threads, same prio, serialized, many tasks"));
            c0.num_tasks = c1.num_tasks = 20;
            app_run(&c0,&s0,&c1,&s1,report_path);
            EXPECT_EQ(2*c0.num_tasks,s0.num_tasks_completed+s1.num_tasks_completed) << "Both apps expected to succeed";
            EXPECT_EQ(0,s0.num_tasks_aborted+s1.num_tasks_aborted) << "No abortion expected";
            EXPECT_EQ(0,s0.num_failed_tests) << "DSP Output should be correct";
            EXPECT_EQ(0,s1.num_failed_tests) << "DSP Output should be correct";
}
TEST(TEST_SUITE_NAME,app_run_test3) {
        mega_cycles_per_task = DEFAULT_NUM_WORKERS * DEFAULT_TRIGGER_PERIOD_MS * CLOCK_SPEED_MHZ * THRESHOLD_MULTIPLIER / 1000;
        mega_cycles_per_task = (num_hvx_units==2)?mega_cycles_per_task/2:mega_cycles_per_task;
        EXPECT_EQ(1,new_test("Two threads, same prio, serialized. Timed thread with timed tasks take longer than their task period."));
        c0.mega_cycles_per_task = c1.mega_cycles_per_task = mega_cycles_per_task;
        printf("DSP task duration: %d ms, i.e. %d x longer than trigger period (%d ms).  Requires %d Mcycles/task\n",
        THRESHOLD_MULTIPLIER*DEFAULT_TRIGGER_PERIOD_MS,THRESHOLD_MULTIPLIER,DEFAULT_TRIGGER_PERIOD_MS,c0.mega_cycles_per_task);
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(0,s0.num_tasks_completed+s1.num_tasks_completed) << "Only one app expected to succeed";
        EXPECT_EQ(2*DEFAULT_NUM_TASKS,s0.num_tasks_aborted+s1.num_tasks_aborted) << "Apps expected to not complete in time and abort";

}
TEST(TEST_SUITE_NAME,app_run_test4) {
        EXPECT_EQ(1,new_test("Same as previous test but tasks are queued (thus period is irrelevant to the app)."));
        c0.task_trigger = c1.task_trigger = 0;
        c0.mega_cycles_per_task = c1.mega_cycles_per_task = mega_cycles_per_task;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(2*DEFAULT_NUM_TASKS,s0.num_tasks_completed+s1.num_tasks_completed) << "Both apps expected to succeed";
}
TEST(TEST_SUITE_NAME,app_run_test5) {
        EXPECT_EQ(1,new_test("Two threads, same prio, serialized, queued. One thread taking longer than resource acquire time out, causing other thread to fail."));
        mega_cycles_per_task = DEFAULT_NUM_WORKERS * DEFAULT_RESOURCE_ACQUIRE_TIMEOUT_US * CLOCK_SPEED_MHZ * THRESHOLD_MULTIPLIER / 1000000;
        mega_cycles_per_task = (num_hvx_units==2)?mega_cycles_per_task/2:mega_cycles_per_task;
        c0.task_trigger = c1.task_trigger = 0;
        c0.mega_cycles_per_task = c1.mega_cycles_per_task = mega_cycles_per_task;
        printf("DSP task duration: %lu us, i.e. %dx longer than resource acquire timeout (%lu us).  Requires %d Mcycles/task\n",
               THRESHOLD_MULTIPLIER*DEFAULT_RESOURCE_ACQUIRE_TIMEOUT_US,THRESHOLD_MULTIPLIER,DEFAULT_RESOURCE_ACQUIRE_TIMEOUT_US,c0.mega_cycles_per_task);
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed+s1.num_tasks_completed) << "Only one app expected to succeed";
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_failed+s1.num_tasks_failed) << "One app expected to timeout and fail";
        EXPECT_EQ(0,s0.num_tasks_aborted+s1.num_tasks_aborted) << "Timing out is considered a failure not an abortion. No app should have aborted";
}
TEST(TEST_SUITE_NAME,app_run_test6) {
        EXPECT_EQ(1,new_test("Same as above, but threads have different priorities.  We can now predict which thread will timeout."));
        c0.task_trigger = c1.task_trigger = 0;
        c0.mega_cycles_per_task = c1.mega_cycles_per_task = mega_cycles_per_task;
        c1.thread_priority = c0.thread_priority - 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(0,s0.num_tasks_completed) << "Lower priority thread should timeout on resource request";
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_failed_tests) << "DSP Output should be incorrect for failed tasks";
        EXPECT_EQ(DEFAULT_NUM_TASKS,s1.num_tasks_completed) << "Higher priority thread should succeed in executing a long task";
        EXPECT_EQ(0,s1.num_failed_tests) << "DSP Output should be correct";
}
TEST(TEST_SUITE_NAME,app_run_test7) {
        EXPECT_EQ(1,new_test("Same as above, permuting roles between threads"));
        c0.task_trigger = c1.task_trigger = 0;
        c0.mega_cycles_per_task = c1.mega_cycles_per_task = mega_cycles_per_task;
        c0.thread_priority = c1.thread_priority - 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed) << "Higher priority thread should succeed in executing a long task";
        EXPECT_EQ(0,s1.num_tasks_completed) << "Lower priority thread should timeout on resource request";
}
TEST(TEST_SUITE_NAME,app_run_test8) {
        EXPECT_EQ(1,new_test("Two threads, diff prio, serialized."));
        c1.thread_priority = c0.thread_priority - 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(2*DEFAULT_NUM_TASKS,s0.num_tasks_completed+s1.num_tasks_completed) << "Both apps expected to succeed";
}
TEST(TEST_SUITE_NAME,app_run_test9) {
        EXPECT_EQ(1,new_test("Two threads, same prio, not serialized."));
        c0.is_serialized = c1.is_serialized = 0;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(2*DEFAULT_NUM_TASKS,s0.num_tasks_completed+s1.num_tasks_completed) << "Both apps expected to succeed";
}
TEST(TEST_SUITE_NAME,app_run_test10) {
        EXPECT_EQ(1,new_test("One thread only"));
        c1.is_off = 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed) << "The only app running is expected to complete";
        EXPECT_EQ(0,s1.num_tasks_completed) << "No tasks to execute for this app";
        EXPECT_EQ(0,s1.num_tasks_aborted) << "No tasks to execute for this app" ;
        EXPECT_EQ(0,s1.num_tasks_failed) << "No tasks to execute for this app";
}
TEST(TEST_SUITE_NAME_ASYNC,app_run_test11) {
        EXPECT_EQ(1,new_test("One thread only, async"));
        c0.verbose_mask=c1.verbose_mask=0x1f;
        c0.is_asynchronous = 1;
        c1.is_off = 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed) << "The only app running is expected to complete";
        EXPECT_EQ(0,s1.num_tasks_completed) << "No tasks to execute for this app";
        EXPECT_EQ(0,s0.num_failed_tests) << "DSP Output should be correct";
}

TEST(TEST_SUITE_NAME_ASYNC,app_run_test12) {
        EXPECT_EQ(1,new_test("One thread async, one thread sync"));
        c0.verbose_mask=c1.verbose_mask=0x1f;
        c0.is_asynchronous = 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed) << "Async app completed properly";
        EXPECT_EQ(DEFAULT_NUM_TASKS,s1.num_tasks_completed) << "Sync app completed properly";
        EXPECT_EQ(0,s0.num_failed_tests) << "DSP Output should be correct for Async app";
        EXPECT_EQ(0,s1.num_failed_tests) << "DSP Output should be correct for Sync app";
}
TEST(TEST_SUITE_NAME_ASYNC,app_run_test13) {
        EXPECT_EQ(1,new_test("Two async threads"));
        c0.verbose_mask=c1.verbose_mask=0x1f;
        c0.is_asynchronous = 1;
        c1.is_asynchronous = 1;
        app_run(&c0, &s0, &c1, &s1, report_path);
        EXPECT_EQ(DEFAULT_NUM_TASKS,s0.num_tasks_completed) << "First async app expected to complete properly" ;
        EXPECT_EQ(DEFAULT_NUM_TASKS,s1.num_tasks_completed) << "Second async app expected to complete properly" ;
}
int main(int argc, char* argv[]) {

    int nErr = 0;
    int nCheck = 0;
    int too_many_errors=0;
    int option=0;
    int is_unsigned=1;
    while((option=getopt(argc,argv,"t:U:")) != -1) {
        switch(option) {
            case 't' : test_to_run = atoi(optarg); printf("Running specific test %d\n",test_to_run);
                break;
            case 'U' : is_unsigned = atoi(optarg);
                break;
            default:
                printf("Usage:\n"
                       "    android_app_test [-t test_to_run] [-U unsigned_PD]\n"
                       "Options:\n\n"
                       "   -t test_to_run : Run all tests when this option is not used. Run only the specified test otherwise.\n"
                       "   -U unsigned_PD : Run on signed or unsigned PD.\n"
                       "        0 : Run on signed PD.\n"
                       "        1 : Run on unsigned PD.\n"
                       "        Default value is 1.\n");
                return -1;
        }
    }
    int retVal = 0;
    if(is_unsigned==1) {
        if(request_unsigned_pd()!=0) {
            exit(-1);
        }
        else {
            LOG("Running on Unsigned PD");
        }
    }
    else {
        LOG("Running on Signed PD");
    }
    struct remote_dsp_capability dsp_cap  = {CDSP_DOMAIN_ID,HVX_SUPPORT_128B,0};
    remote_handle_control(DSPRPC_GET_DSP_INFO,&dsp_cap,sizeof(struct remote_dsp_capability));
    num_hvx_units = dsp_cap.capability;
    int n_argc =1;
    char *n_argv[] = {NULL,NULL};
    ::testing::InitGoogleTest(&n_argc,n_argv);
    if(test_to_run==-1) {
        ::testing::GTEST_FLAG(filter) = "*";
    }
    else {
        ::testing::GTEST_FLAG(filter) = (std::string(TOSTRING(TEST_SUITE_NAME))+std::string(".app_run_test")+std::to_string(test_to_run)).c_str();
    }
    ::testing::GTEST_FLAG(filter) = "-async_tests.*:* \0"; //excludes async_tests from running
    retVal = RUN_ALL_TESTS();
    if(retVal!=0) {
        LOG("Error 0x%x",retVal);
    }
    else {
        LOG("Success");
    }
    return retVal;
}
