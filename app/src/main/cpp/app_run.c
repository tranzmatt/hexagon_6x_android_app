/*==============================================================================
  Copyright (c) 2021 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include "AEEStdErr.h"
#include "android_app.h"
#include "android_app_async.h"
#include "app_run.h"
#include "rpcmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "remote.h"
#include "unistd.h"

#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include "logs.h"
#include <dlfcn.h>
#include <string.h>

#include <stdatomic.h>

// Asynchronous call data structure
typedef struct {
    struct fastrpc_async_descriptor desc;    // Asynchronous RPC call descriptor
    int result;                              // Return status
    int array_size;
    uint8_t* in_ptr;
    uint8_t* out_ptr;
    atomic_int* num_failed_tests;
    int session_id;
    pthread_mutex_t* async_completion_mutex;
    pthread_cond_t* async_completion_condition;
    atomic_int* async_num_tasks_completed;
    atomic_int* async_num_tasks_submitted;
    sem_t* async_semaphore;
} async_call_context_t;

typedef struct {
    android_app_configs* configs;
    android_app_stats* stats;
    remote_handle64* handle_app;  // handle shared between the async and sync implementation
} session_data_t;

#pragma weak remote_session_control
#pragma weak remote_handle_control
#pragma weak fastrpc_release_async_job

static remote_handle64 handle_app_0=0;
static remote_handle64 handle_app_1=0;
static int volatile terminate=0;

static  AEEResult (*stub_terminates[2])(remote_handle64 handle) = {NULL,NULL} ;


#define MAX_PENDING_ASYNC_TASKS 4

int request_unsigned_pd() {
    if (remote_session_control) {
        struct remote_rpc_control_unsigned_module data;
        data.enable = 1;
        data.domain = CDSP_DOMAIN_ID;
        int fastrpc_response=0;
        if (0 !=
            (fastrpc_response = remote_session_control(DSPRPC_CONTROL_UNSIGNED_MODULE, (void *) &data,
                                                       sizeof(data)))) {
            LOG("ERROR: remote_session_control failed for CDSP, returned 0x%x. This is unexpected.", fastrpc_response);
            return fastrpc_response;
        }
    } else {
        LOG("WARNING: remote_session_control interface is not supported on this device. Proceeding but device needs to be signed.");
    }
    return AEE_SUCCESS;
}

unsigned long long diff_time(struct timeval *start, struct timeval * end)
{
    unsigned long long s = start->tv_sec * 1000000ULL + start->tv_usec;
    unsigned long long e = end->tv_sec * 1000000ULL + end->tv_usec;
    return (e - s);
}

void
print_report(const android_app_configs *c0, const android_app_configs *c1, android_app_stats *s0,
             android_app_stats *s1, const char *report_path) {

    FILE *file_pointer = NULL;
    if (report_path) {
        file_pointer = fopen(report_path, "a");
        if (file_pointer) {
            LOG("Writing logs to %s", report_path);
        }
    } else {
        const char* report_path_bin = "/vendor/bin/android_app_reports.txt";
        file_pointer = fopen(report_path_bin, "a");
        if (file_pointer) {
            LOG("Writing logs to %s",report_path_bin);
        } else {
            LOG("Unable to write to either %s or %s",report_path_bin,report_path);
        }
    }


    int num_tasks_0 = s0->num_tasks_completed+s0->num_tasks_aborted+s0->num_tasks_failed;
    float fastrpc_overhead_per_call_0 = 0;
    float avg_num_tasks_completed_per_second_0 = 0;
    float percent_dsp_time_us_waiting_on_resource_0 = 0;
    float dsp_process_time_ms_0 = 0;
    if (num_tasks_0) {
        fastrpc_overhead_per_call_0 = (s0->cpu_time_us-s0->dsp_time_us)/(float)num_tasks_0;
        if ((c0->task_trigger) && ((c0->is_asynchronous))) {
            // A FastRPC overhead in the context of periodical event is not defined
            fastrpc_overhead_per_call_0=-1;
        }
        avg_num_tasks_completed_per_second_0 = s0->num_tasks_completed/(float)s0->cpu_time_us * 1000000;
        percent_dsp_time_us_waiting_on_resource_0 = 100*s0->waiting_on_resource_time_us/(float)s0->dsp_time_us;
        dsp_process_time_ms_0 = (float)(s0->dsp_time_us-s0->waiting_on_resource_time_us)/1000;
    }

    int num_tasks_1 = s1->num_tasks_completed+s1->num_tasks_aborted+s1->num_tasks_failed;
    float fastrpc_overhead_per_call_1 = 0;
    float avg_num_tasks_completed_per_second_1 = 0;
    float percent_dsp_time_us_waiting_on_resource_1 = 0;
    float dsp_process_time_ms_1 = 0;
    if (num_tasks_1) {
        fastrpc_overhead_per_call_1 = (s1->cpu_time_us-s1->dsp_time_us)/(float)num_tasks_1;
        if ((c1->task_trigger) && ((c1->is_asynchronous))) {
            // A FastRPC overhead in the context of periodical event is not defined
            fastrpc_overhead_per_call_1=-1;
        }
        avg_num_tasks_completed_per_second_1 = s1->num_tasks_completed/(float)s1->cpu_time_us * 1000000;
        percent_dsp_time_us_waiting_on_resource_1 = 100*s1->waiting_on_resource_time_us/(float)s1->dsp_time_us;
        dsp_process_time_ms_1 = (float)(s1->dsp_time_us-s1->waiting_on_resource_time_us)/1000;
    }

    if (c0->verbose_mask) {
        LOG_AND_SAVE("|>-------------------------------------------------------------------------------------\n");
        LOG_AND_SAVE("|>Session configurations");
        LOG_AND_SAVE("|>  Session id          %10d %10d", c0->session_id, c1->session_id);
        LOG_AND_SAVE("|>  VTCM size (MB)          %10d %10d", c0->vtcm_size_mb, c1->vtcm_size_mb);
        LOG_AND_SAVE("|>  Num workers             %10d %10d", c0->num_workers, c1->num_workers);
        LOG_AND_SAVE("|>  MCycles per task        %10d %10d", c0->mega_cycles_per_task, c1->mega_cycles_per_task);
        LOG_AND_SAVE("|>  Turned off?             %10s %10s", c0->is_off ? "Yes" : "No",
            c1->is_off ? "Yes" : "No");
        LOG_AND_SAVE("|>  Serialized?             %10s %10s", c0->is_serialized ? "Yes" : "No",
            c1->is_serialized ? "Yes" : "No");
        LOG_AND_SAVE("|>  Asynchronous?           %10s %10s", c0->is_asynchronous ? "Yes" : "No",
            c1->is_asynchronous ? "Yes" : "No");
        LOG_AND_SAVE("|>  Trigger period (ms)     %10d %10d", c0->trigger_period_ms, c1->trigger_period_ms);
        LOG_AND_SAVE("|>  Res acq timeout (us)    %10d %10d", c0->resource_acquire_timeout_us, c1->resource_acquire_timeout_us);
        LOG_AND_SAVE("|>  Trigger type            %10s %10s", c0->task_trigger ? "Periodical" : "Queued",
            c1->task_trigger ? "Periodical" : "Queued");
        LOG_AND_SAVE("|>  Thread priority         %10d %10d", c0->thread_priority, c1->thread_priority);
        LOG_AND_SAVE("|>  Num tasks               %10d %10d", c0->num_tasks, c1->num_tasks);
        LOG_AND_SAVE("|>Session statistics");
        LOG_AND_SAVE("|>  FastRPC overhead  (us)  %10.0f %10.0f", fastrpc_overhead_per_call_0,
            fastrpc_overhead_per_call_1);
        LOG_AND_SAVE("|>  Tasks per second        %10.2f %10.2f", avg_num_tasks_completed_per_second_0,
            avg_num_tasks_completed_per_second_1);
        LOG_AND_SAVE("|>  %%time wait on resource  %10.2f %10.2f", percent_dsp_time_us_waiting_on_resource_0,
            percent_dsp_time_us_waiting_on_resource_1);
        LOG_AND_SAVE("|>  DSP compute time (ms)   %10.1f %10.1f", dsp_process_time_ms_0, dsp_process_time_ms_1);
        LOG_AND_SAVE("|>  Num tasks completed     %10d %10d", s0->num_tasks_completed, s1->num_tasks_completed);
        LOG_AND_SAVE("|>  Num tasks aborted       %10d %10d", s0->num_tasks_aborted, s1->num_tasks_aborted);
        LOG_AND_SAVE("|>  Num tasks failed        %10d %10d", s0->num_tasks_failed, s1->num_tasks_failed);
        LOG_AND_SAVE("|>  Num tasks interrupted   %10d %10d", s0->num_tasks_interrupted,
            s1->num_tasks_interrupted);
        LOG_AND_SAVE("|>  Num interruptions       %10d %10d", s0->num_interruptions, s1->num_interruptions);
    }
    if (file_pointer) fclose(file_pointer);
}



/* Test the correctness of the output buffer
 * In a larger session, this function would be where we could make a copy of the output buffer
 * for further processing.
 */
int verify(int session_id, uint8_t* in_ptr, uint8_t* out_ptr, int array_size) {
    int found_error=0;
    for (int i=0; i < array_size; i++) {
        if (in_ptr[i] != out_ptr[i]) {
            //LOG("ERROR: Verification of the async_call_context function call failed on index %d: %d != %d", i, in_ptr[i], out_ptr[i]);
            found_error=1;
        }
    }
    return found_error;
}


void async_callback_fn(fastrpc_async_jobid jobid, void *context, int result) {

    async_call_context_t *async_call_context = (async_call_context_t*)context;

    //LOG("Completed one async task...");

    if (async_call_context) {
        //LOG("Running callback for job id 0x%llx 0x%llx result 0x%x\n", (uint64_t)jobid, (uint64_t)async_call_context->desc.jobid, result);
        async_call_context->result = result;
    } else {
        LOG("ERROR: NULL context passed to callback function");
    }
    if (result) {
        LOG("ERROR: callback failed for job id 0x%llx result 0x%x\n", (uint64_t)jobid, result);
    }

    int failed_test = verify(async_call_context->session_id,
                             async_call_context->in_ptr, async_call_context->out_ptr, async_call_context->array_size);
    atomic_fetch_add(async_call_context->num_failed_tests,failed_test);

    atomic_fetch_add(async_call_context->async_num_tasks_completed,1);

    if (atomic_load(async_call_context->async_num_tasks_completed) == atomic_load(async_call_context->async_num_tasks_submitted)) {
        pthread_cond_signal(async_call_context->async_completion_condition);
    }
    pthread_mutex_unlock(async_call_context->async_completion_mutex);

    // Enable a new async call to be made
    fastrpc_release_async_job(jobid);
    sem_post(async_call_context->async_semaphore);
}

static void* launch_session(void *data) {

    session_data_t *session_data = (session_data_t *)data;
    *session_data->stats = (android_app_stats) {0};
    int fastrpc_response = AEE_SUCCESS;
    uint8_t* in_ptr = NULL;
    uint8_t* out_ptr = NULL;
    struct timeval start_task, end_task;
    struct timeval start_app, end_app;
    uint64 cpu_time_last_iteration_us = 0;
    uint64 cpu_time_us = 0;
    char *uri = NULL;
    atomic_int num_failed_tests;
    atomic_init(&num_failed_tests,0);
    void *android_app_lib = NULL;

    if ((session_data->configs->is_off)||(session_data->configs->num_tasks<=0)) {
        //LOG("No tasks to execute for app %d\n",session_data->configs->session_id);
        return NULL;
    }

    int array_size = session_data->configs->array_width * session_data->configs->array_height;
    if (array_size<=0) {
        LOG("ERROR: Invalid array size: %d",array_size);
        return NULL;
    }

    android_app_configs* c = (android_app_configs*) rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, sizeof(android_app_configs));
    *c=*session_data->configs;

    LOG("INFO: Launching %ssynchronous session %d",c->is_asynchronous?"a":"", c->session_id);

    AEEResult (*stub_open)(const char *uri, remote_handle64 *handle) = NULL;
    AEEResult (*stub_set_clocks)(remote_handle64 h, int32 power_level, int32 latency, int32 dcvs_enabled) = NULL;
    AEEResult (*stub_get_stats)(remote_handle64 handle, android_app_stats* stats) = NULL;
    AEEResult (*stub_run)(remote_handle64 handle, const android_app_configs* configs, const uint8* in_ptr, int in_length, uint8* out_ptr, int out_length) = NULL;
    AEEResult (*stub_async_run)(remote_handle64 handle, fastrpc_async_descriptor_t* async_desc, const android_app_configs* configs, const uint8* in_ptr, int in_length, uint8* out_ptr, int out_length) = NULL;
    AEEResult (*stub_close)(remote_handle64 handle) = NULL;

    if (c->is_asynchronous) {
        android_app_lib = dlopen("libandroid_app_async.so", RTLD_NOW);
        if (android_app_lib) {
            stub_open = dlsym(android_app_lib, "android_app_async_open");
            stub_set_clocks = dlsym(android_app_lib, "android_app_async_set_clocks");
            stub_get_stats = dlsym(android_app_lib, "android_app_async_get_stats");
            stub_terminates[c->session_id-1] = dlsym(android_app_lib, "android_app_async_terminate");
            stub_async_run = dlsym(android_app_lib, "android_app_async_run");
            stub_close = dlsym(android_app_lib, "android_app_async_close");
            if ((!stub_open&&stub_set_clocks&&stub_get_stats&&stub_terminates[c->session_id-1]&&stub_async_run&&stub_close)) {
                LOG("ERROR: At least one symbol is missing from the async stub");
                goto bail;
            }
        } else {
            LOG("ERROR: Unable to load asynchronous stub library");
            goto bail;
        }
    } else {
        android_app_lib = dlopen("libandroid_app.so", RTLD_NOW);
        if (android_app_lib) {
            stub_open = dlsym(android_app_lib, "android_app_open");
            stub_set_clocks = dlsym(android_app_lib, "android_app_set_clocks");
            stub_get_stats = dlsym(android_app_lib, "android_app_get_stats");
            stub_terminates[c->session_id-1] = dlsym(android_app_lib, "android_app_terminate");
            stub_run = dlsym(android_app_lib, "android_app_run");
            stub_close = dlsym(android_app_lib, "android_app_close");
            if ((!stub_open && stub_set_clocks && stub_get_stats && stub_terminates[c->session_id-1] && stub_run &&
                 stub_close)) {
                LOG("ERROR: At least one symbol is missing from the sync stub");
                goto bail;
            }
        } else {
            LOG("ERROR: Unable to load synchronous stub library");
            goto bail;
        }
    }


    /* We rely on the gradle file to append the DSP version to the library name we build.
   * We default to running on the CDSP only even though the code can easily be extended
   * to run on other DSPs.  However, only the CDSP support unsigned domains.
   */

    //pid_t pid = getpid();
    //LOG("---------- Calling open for session %d with handle = %lu on process id %d",c->session_id,*session_data->handle_app,pid)

    /* Set the URI to load the required shared object depending on the operating mode (synchronous/asynchronous) and the
     * CDSP version (V68/V69/V73/V75).
     *
     * Eight separate shared objects are available to choose from:
     * V68 synchronous flavor
     * V68 asynchronous flavor
     * V69 synchronous flavor
     * V69 asynchronous flavor
     * V73 synchronous flavor
     * V73 asynchronous flavor
     * V75 synchronous flavor
     * V75 asynchronous flavor
     */
    char* uri_old=NULL;
    if (c->is_asynchronous) {
        uri = android_app_async_URI CDSP_DOMAIN;
    } else {
        uri = android_app_URI CDSP_DOMAIN;
    }
    char* uri_after_dot_so = strchr(uri,'.');
    if (uri_after_dot_so==NULL) {
        LOG("ERROR: Unexpected format for uri %s", uri);
        goto bail;
    }
    char* dsp_version_str = NULL;
    if (c->dsp_version == 0x68)
        dsp_version_str = "_v68";
    else if (c->dsp_version == 0x69)
        dsp_version_str = "_v69";
    else if (c->dsp_version == 0x73)
        dsp_version_str = "_v73";
    else if (c->dsp_version == 0x75)
        dsp_version_str = "_v75";
    else {
        LOG("ERROR: Unsupported DSP version %x",c->dsp_version);
        goto bail;
    }

    int length_dsp_version = strlen(dsp_version_str);
    int length_uri = strlen(uri);
    int length_uri_after_dot = strlen(uri_after_dot_so);
    int length_uri_before_dot = length_uri - length_uri_after_dot;
    char* uri_with_dsp_version = malloc(length_uri+length_dsp_version+1);
    memcpy(uri_with_dsp_version,uri, length_uri_before_dot);
    memcpy(uri_with_dsp_version+length_uri_before_dot,dsp_version_str, length_dsp_version);
    memcpy(uri_with_dsp_version+length_uri_before_dot+length_dsp_version,uri_after_dot_so, length_uri_after_dot+1);

    if (AEE_SUCCESS != (fastrpc_response = stub_open(uri_with_dsp_version, session_data->handle_app))) {
        LOG("ERROR: Unable to launch session %d with skel %s: %0x\n", c->session_id, uri_with_dsp_version, fastrpc_response);
        goto bail;
    }

    int power_level = 7;
    int dcvs_enabled = 0;
    int latency = 100;
    fastrpc_response=stub_set_clocks(*session_data->handle_app, power_level, latency, dcvs_enabled);
    if (fastrpc_response != AEE_SUCCESS) {
        LOG("ERROR %0x: Failed to set DSP power mode",fastrpc_response);
    }

    // We use a circular buffer to enable an arbitrary number of tasks to run
    int num_buffers = c->num_tasks>MAX_PENDING_ASYNC_TASKS?MAX_PENDING_ASYNC_TASKS:c->num_tasks;
    in_ptr = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, num_buffers * array_size);
    out_ptr = (uint8_t *)rpcmem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, num_buffers * array_size );
    for (int i=0; i < num_buffers * array_size; i++) {
        in_ptr[i]=i;
        out_ptr[i]=0;
    }

    async_call_context_t *async_call_contexts = NULL;

    pthread_mutex_t async_completion_mutex;
    pthread_cond_t async_completion_condition;
    atomic_int async_num_tasks_completed;;
    atomic_int async_num_tasks_submitted;
    atomic_init(&async_num_tasks_completed,0);
    atomic_init(&async_num_tasks_submitted,0);
    sem_t async_semaphore;

    if (c->is_asynchronous) {
        async_call_contexts = (async_call_context_t *) calloc(num_buffers,
                                                              sizeof(async_call_context_t));
        sem_init(&async_semaphore, 0, MAX_PENDING_ASYNC_TASKS);
        pthread_mutex_init(&async_completion_mutex, NULL);
        pthread_cond_init(&async_completion_condition, NULL);
        for (int i=0;i<num_buffers;i++) {
            async_call_contexts[i].desc.type = FASTRPC_ASYNC_CALLBACK;
            async_call_contexts[i].desc.cb.fn = async_callback_fn;
            async_call_contexts[i].desc.cb.context = &async_call_contexts[i];
            async_call_contexts[i].session_id = c->session_id;
            async_call_contexts[i].in_ptr=in_ptr + i * array_size;
            async_call_contexts[i].out_ptr=out_ptr + i * array_size;

            async_call_contexts[i].async_semaphore = &async_semaphore;
            async_call_contexts[i].async_completion_mutex = &async_completion_mutex;
            async_call_contexts[i].async_completion_condition = &async_completion_condition;
            async_call_contexts[i].async_num_tasks_completed = &async_num_tasks_completed;
            async_call_contexts[i].async_num_tasks_submitted = &async_num_tasks_submitted;
            async_call_contexts[i].num_failed_tests = &num_failed_tests;
        }
    }

    int circular_buffer_index=0;
    gettimeofday(&start_app, 0);
    for (int i=0;i<c->num_tasks;i++) {

        if (circular_buffer_index==num_buffers) {
            circular_buffer_index=0;
        }

        if (terminate) {
            break;
        }

        gettimeofday(&start_task, 0);

        if (c->is_asynchronous) {
            sem_wait(&async_semaphore);

            //LOG("Called sync run for session %d",c->session_id);
            fastrpc_response = stub_async_run(*session_data->handle_app, &async_call_contexts[circular_buffer_index].desc, c, async_call_contexts[circular_buffer_index].in_ptr, array_size, async_call_contexts[circular_buffer_index].out_ptr, array_size);
            if (fastrpc_response == 0) {
                atomic_fetch_add(&async_num_tasks_submitted,1);
            } else {
                LOG("ERROR: Failed to submit Async call with error 0x%x for session %d", fastrpc_response,c->session_id);
                goto bail;
            }
        } else {
            fastrpc_response = stub_run(*session_data->handle_app, c, in_ptr + circular_buffer_index*array_size, array_size, out_ptr + circular_buffer_index*array_size, array_size);
            if (fastrpc_response != AEE_SUCCESS) {
                LOG("ERROR %0x: Failed to run for session %d",fastrpc_response,c->session_id);
            }
        }

        gettimeofday(&end_task, 0);
        cpu_time_last_iteration_us = diff_time(&start_task, &end_task);
        cpu_time_us+=cpu_time_last_iteration_us;


        if (!c->is_asynchronous) {
            // An atomic variable is not needed for synchronous cases
            int failed_test = verify(c->session_id,
                                     in_ptr + circular_buffer_index * array_size,
                                     out_ptr + circular_buffer_index * array_size, array_size);
            atomic_fetch_add(&num_failed_tests,failed_test);
        }

        if (c->task_trigger==1) { // TODO: Replace 1 with Java TaskTrigger.TIMED
            int wait_time_us = c->trigger_period_ms*1000-cpu_time_last_iteration_us;
            if (wait_time_us>0) {
                //LOG("Sleeping for %d us before launching next task on app %d",wait_time_us,c->session_id);
                usleep(wait_time_us);
            } else {
                // LOG("No need to sleep: c->task_trigger=%d, c->trigger_period_ms=%d, cpu_time_last_iteration_us=%lld on app %d",c->task_trigger,c->trigger_period_ms,cpu_time_last_iteration_us,c->session_id);
            }
        }
        circular_buffer_index+=1;
    }

    if (c->is_asynchronous) {
        gettimeofday(&start_task, 0);
        pthread_mutex_lock(&async_completion_mutex);
        if (atomic_load(&async_num_tasks_completed) < atomic_load(&async_num_tasks_submitted)) {
            LOG("Only %d async tasks completed out of %d submitted for session %d. Waiting for remaining task(s).",
                atomic_load(&async_num_tasks_completed),atomic_load(&async_num_tasks_submitted),
                c->session_id);
            pthread_cond_wait(&async_completion_condition, &async_completion_mutex);
        }
        pthread_mutex_unlock(&async_completion_mutex);
        gettimeofday(&end_task, 0);
        cpu_time_us += (int) diff_time(&start_task, &end_task);
        LOG("All async tasks completed for session %d.",c->session_id);
    }

    gettimeofday(&end_app, 0);

    fastrpc_response = stub_get_stats(*session_data->handle_app, session_data->stats);
    if (fastrpc_response != AEE_SUCCESS) {
        LOG("ERROR %0x: Failed to get stats",fastrpc_response);
    }

    session_data->stats->num_failed_tests = atomic_load(&num_failed_tests);
    session_data->stats->test_time_us = diff_time(&start_app, &end_app);
    session_data->stats->session_id=c->session_id;
    session_data->stats->cpu_time_us=cpu_time_us;

    if (AEE_SUCCESS != (fastrpc_response = stub_close(*session_data->handle_app))) {
        LOG("ERROR 0x%x: Failed to close handle for session %d\n", fastrpc_response,
            c->session_id);
    }

    bail:

    LOG("---------------------- Completed session %d",c->session_id);
    stub_terminates[c->session_id-1]=NULL;
    if (android_app_lib) dlclose(android_app_lib);
    if(c) rpcmem_free(c);
    if(in_ptr) rpcmem_free(in_ptr);
    if(out_ptr) rpcmem_free(out_ptr);
    if(uri_with_dsp_version) free(uri_with_dsp_version);
    if(async_call_contexts) free(async_call_contexts);
    return NULL;

}

int app_terminate() {
    if(stub_terminates[0]) {
        stub_terminates[0](handle_app_0);
    } else {
        LOG("INFO: No action to take to terminate session 1");
    }
    if(stub_terminates[1]) {
        stub_terminates[1](handle_app_1);
    } else {
        LOG("INFO: No action to take to terminate session 2");
    }
    return AEE_SUCCESS;
}

int app_run(android_app_configs *c0, android_app_stats *s0, android_app_configs *c1,
            android_app_stats *s1, const char *cache_path) {
    int fastrpc_response = AEE_SUCCESS;
    pthread_attr_t attr;
    pthread_t app_0_thread, app_1_thread;
    session_data_t session_data_0, session_data_1;

    int dsp_version=0;
    if(remote_handle_control){
        struct remote_dsp_capability dsp_capability_domain_arch_version = {CDSP_DOMAIN_ID, ARCH_VER, 0};
        int rc = remote_handle_control(DSPRPC_GET_DSP_INFO, &dsp_capability_domain_arch_version, sizeof(struct remote_dsp_capability));
        if ( rc != 0) {
            LOG("Error %x while querying DSP version\n",rc);
            return rc;
        } else {
            dsp_version=0xff&dsp_capability_domain_arch_version.capability;
        }
    } else {
        LOG("ERROR: remote_handle_control support cannot be found.");
        return -1;
    }

    if (dsp_version == 0x68 || dsp_version == 0x69 || dsp_version == 0x73 || dsp_version == 0x75) {
        LOG("INFO: Identified DSP Arch version as 0x%x", dsp_version);
    } else {
        LOG("ERROR: DSP Arch version 0x%x is not supported", dsp_version);
        return -1;
    }
    c0->dsp_version=dsp_version;
    c1->dsp_version=dsp_version;

    session_data_0.configs=c0;
    session_data_0.stats=s0;
    handle_app_0 = 0;
    session_data_0.handle_app = &handle_app_0;

    session_data_1.configs=c1;
    session_data_1.stats=s1;
    handle_app_1 = 0;
    session_data_1.handle_app = &handle_app_1;

/*
  // Needs API level 30 so skipping for now.   All logs will be visible.
  if (c0->verbose_mask) {
    __android_log_set_minimum_priority(ANDROID_LOG_DEBUG);
  }
*/

    int response;
    if ((response = pthread_attr_init(&attr)) != 0) {
        LOG("ERROR: pthread_attr_init failed: %0x",response);
        goto deinit;
    }
    if ((response = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE)) != 0) {
        LOG("ERROR: pthread_attr_setdetachstate failed: %0x", response);
        goto deinit;
    }
    if ((response = pthread_create(&app_0_thread, &attr, launch_session, &session_data_0)) != 0) {
        LOG("ERROR: app_0 pthread_create failed: %0x", response);
        goto deinit;
    }
    if ((response = pthread_create(&app_1_thread, &attr, launch_session, &session_data_1)) != 0) {
        LOG("ERROR: app_1 pthread_create failed: %0x", response);
        goto deinit;
    }
    void *ret;
    if (((response = pthread_join(app_0_thread, &ret)) != 0) | (ret != NULL)) {
        LOG("ERROR: app_0 pthread_join failed: %0x", response);
        goto deinit;
    }
    if (((response = pthread_join(app_1_thread, &ret)) != 0) | (ret != NULL)) {
        LOG("ERROR: app_1 pthread_join failed: %0x", response);
        goto deinit;
    }
    terminate=0;

    if (c0->verbose_mask) {
        print_report(c0, c1, s0, s1, cache_path);
    }

    deinit:
    return fastrpc_response;
}
