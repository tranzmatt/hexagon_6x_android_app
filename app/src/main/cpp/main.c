/*==============================================================================
  Copyright (c) 2021,2022, 2024 Qualcomm Technologies, Inc.
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
#include "android_app_defs.h"
#include "logs.h"

static void print_usage() {
    printf("Usage:\n"
           "    android_app [-t trigger_period] [-w no_workers] [-n no_tasks] [-p priority_delta]\n"
           "    [-s use_serialized_requests] [-m mega_cycles_per_task] [-k rpc_mode] [-U unsigned_PD]\n"
           "Options:\n\n"
           " -t trigger_period : Time period between completion of one task and start of the next task on DSP.\n"
              "If this option is not specified, requests will be queued."
           " -w no_workers : No of DSP software threads launched to execute a DSP task.\n"
               "Default value : 4\n"
           " -n no_tasks : No of DSP tasks, each corresponding to one FastRPC call.\n"
               "Default value : 2\n"
           " -p priority_delta : The difference in priority between the threads executing the second and first DSP tasks.\n"
               "Default value : 1\n"
           " -s use_serialized_requests : Enable serialized requests.\n"
               "Default value : 1\n"
           " -m mega_cycles_per_task : The no of cycles that each DSP task must consume.\n"
               "Default value : 100\n"
           " -k rpc_mode : Select the RPC Mode (0 - Regular FastRPC, 1 - Asynchronous FastRPC)\n"
           " -U unsigned_pd : Run on signed or unsigned PD.\n"
               "0 : Run on signed PD.\n"
               "1 : Run on unsigned PD.\n"
               "Default value : 1\n");
}

int main(int argc, char* argv[])
{
    android_app_configs c0={0};
    android_app_configs c1={0};
    android_app_stats s0;
    android_app_stats s1;
    int option=0;
    int trigger_period_ms = 0;
    int num_workers= 4;
    int num_tasks= 2;;
    int priority_delta= 1;
    int is_serialized= 1;
    int mega_cycles_per_task= 100;
    int is_asynchronous= 0;
    int is_periodical= 0;
    int vtcm_size_mb=256;
    int resource_acquire_timeout_us=100000;
    int is_unsigned= 1;

    while((option=getopt(argc,argv,"t:w:n:p:s:m:k:U:")) != -1) {
        switch(option) {
            case 't' : trigger_period_ms = atoi(optarg);
                break;
            case 'w' : num_workers = atoi(optarg);
                break;
            case 'n' : num_tasks = atoi(optarg);
                break;
            case 'p' : priority_delta = atoi(optarg);
                break;
            case 's' : is_serialized  = atoi(optarg);
                break;
            case 'm' : mega_cycles_per_task = atoi(optarg);
                break;
            case 'k' : is_asynchronous = atoi(optarg);
                break;
            case 'U' : is_unsigned = atoi(optarg);
                break;
            default :
               print_usage();
               return -1;
        }
    }

    c0.vtcm_size_mb=vtcm_size_mb;
    c0.num_workers=num_workers;
    c0.mega_cycles_per_task=mega_cycles_per_task;
    c0.is_serialized=is_serialized;
    c0.trigger_period_ms=trigger_period_ms;
    c0.thread_priority = 128;
    c0.num_tasks=num_tasks==0?1:num_tasks;
    c0.is_asynchronous=0;
    c0.array_width=8;
    c0.array_height=8;
    c0.is_asynchronous=is_asynchronous;
    c0.session_id=1;
    c0.verbose_mask=0x1f;
    c0.resource_acquire_timeout_us=resource_acquire_timeout_us;
    c0.task_trigger=is_periodical;

    c1.vtcm_size_mb=vtcm_size_mb;
    c1.num_workers=num_workers;
    c1.mega_cycles_per_task=mega_cycles_per_task;
    c1.is_serialized=is_serialized;
    c1.trigger_period_ms=trigger_period_ms;
    c1.thread_priority = 128 + priority_delta;
    c1.num_tasks=num_tasks;
    c1.is_asynchronous=0;
    c1.array_width=8;
    c1.array_height=8;
    c1.is_asynchronous=is_asynchronous;
    c1.session_id=2;
    c1.verbose_mask=0x1f;
    c1.resource_acquire_timeout_us=resource_acquire_timeout_us;
    c1.task_trigger=is_periodical;

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

    return app_run(&c0,&s0,&c1,&s1,"/vendor/bin/report.txt");

}
