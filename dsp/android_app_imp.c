
/*==============================================================================
  Copyright (c) 2012-2020-2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
  ==============================================================================*/

#include "android_app.h"
#include "remote.h"

#include "hexagon_types.h"
#include "hexagon_protos.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <HAP_power.h>
#include "HAP_farf.h"
#undef FARF_HIGH
#define FARF_HIGH 1
#include "HAP_perf.h"
#include <AEEStdErr.h>

#include "worker_pool.h"
#include "HAP_compute_res.h"
#include <qurt.h>
#include <qurt_thread.h>
#include "android_app_imp_defs.h"

//int on_simulator;

#pragma weak HAP_get_chip_family_id

extern uint32_t HAP_get_chip_family_id(void);
extern void wait_pcycles(int numCycles);

typedef struct
{
  worker_pool_context_t* worker_pool_context;
  android_app_stats*          stats;
  int terminate;
} android_app_handle;

typedef struct
{
  worker_synctoken_t* token;
  unsigned char*      src;
  unsigned char*      dst;
  int                 num_worker_task_cycles;
  unsigned int        num_worker_tasks;
  const android_app_configs*  configs;
  int                 is_task_aborted;
  int                 is_task_failed;
  int                 is_task_interrupted;
  uint64              start_time;
  volatile int        terminate;
} android_app_worker_callback_t;

int android_app_release_callback( unsigned int context_id, void* client_context) {
  android_app_worker_callback_t *data = (android_app_worker_callback_t *)client_context;
  FARF(RUNTIME_HIGH, "Release resource requested for session %d",data->configs->session_id);
  data->is_task_interrupted = 1;
  return 0;
}

AEEResult android_app_set_clocks(remote_handle64 h, int32 power_level, int32 latency, int32 dcvs_enabled) {

  FARF(ALWAYS, "----------- Entering power set clocks");

  // Set client class (useful for monitoring concurrencies)
  HAP_power_request_t request;
  memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
  request.type = HAP_power_set_apptype;
  request.apptype = HAP_POWER_COMPUTE_CLIENT_CLASS;

  void *benchmark_ctx = (void*) (h);
  int retval = HAP_power_set(benchmark_ctx, &request);
  if (retval)  {
    FARF(RUNTIME_HIGH, "Failed first power vote");
    return AEE_EFAILED;
  }

  // Configure clocks & DCVS mode
  memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated.
  request.type = HAP_power_set_DCVS_v2;

  request.dcvs_v2.dcvs_enable = TRUE;
  request.dcvs_v2.dcvs_params.target_corner = power_level;

  if (dcvs_enabled) {
    request.dcvs_v2.dcvs_params.min_corner = HAP_DCVS_VCORNER_DISABLE; // no minimum
    request.dcvs_v2.dcvs_params.max_corner = HAP_DCVS_VCORNER_DISABLE; // no maximum
  } else {
    request.dcvs_v2.dcvs_params.min_corner = request.dcvs_v2.dcvs_params.target_corner;  // min corner = target corner
    request.dcvs_v2.dcvs_params.max_corner = request.dcvs_v2.dcvs_params.target_corner;  // max corner = target corner
  }

  request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
  request.dcvs_v2.set_dcvs_params = TRUE;
  request.dcvs_v2.set_latency = TRUE;
  request.dcvs_v2.latency = latency;
  retval = HAP_power_set(benchmark_ctx, &request);
  if (retval) {
    FARF(RUNTIME_HIGH, "Failed to vote for performance mode");
    return AEE_EFAILED;
  }

  // Note: Explicit HVX power vote is no longer required starting with Lahaina
  memset(&request, 0, sizeof(HAP_power_request_t)); //Important to clear the structure if only selected fields are updated
  request.type = HAP_power_set_HVX;
  request.hvx.power_up = TRUE;
  retval = HAP_power_set(benchmark_ctx, &request);
  if (retval) {
    FARF(RUNTIME_HIGH, "Failed to vote for HVX power");
    return AEE_EFAILED;
  }

  return AEE_SUCCESS;
}


/* Task performed by all workers */
static void android_app_worker_callback(void* data)
{
  android_app_worker_callback_t    *dptr = (android_app_worker_callback_t*)data;
  const android_app_configs* configs = dptr->configs;
  FARF(RUNTIME_HIGH,"Worker launched for session %d",configs->session_id);

  while(((int)worker_pool_atomic_dec_return(&(dptr->num_worker_tasks))) >=0) {
    uint64 current_time = HAP_perf_get_time_us();
    if ((int)(current_time - dptr->start_time)>configs->trigger_period_ms*1000 && configs->task_trigger==1) {  // TODO: Replace 1 with Java TaskTrigger.QUEUED
      FARF(RUNTIME_HIGH, "Aborting worker task %d on session %d: Took longer than %d ms",dptr->num_worker_tasks,configs->session_id, configs->trigger_period_ms);
      dptr->is_task_aborted=1;
      break;
    }
    if (dptr->is_task_interrupted) {
      FARF(RUNTIME_HIGH, "Honoring task interruption request on session %d",configs->session_id);
      break;
    }
    wait_pcycles(dptr->num_worker_task_cycles);
    if (dptr->terminate) {
      FARF(RUNTIME_HIGH, "Aborting task while executing worker task %d: Requested by client",dptr->num_worker_tasks);
      FARF(RUNTIME_HIGH, "dptr = %p, session id = %d",dptr, configs->session_id);
      dptr->is_task_aborted=1;
      break;
    }
  }

  worker_pool_synctoken_jobdone(dptr->token);
}

AEEResult android_app_open(const char*uri, remote_handle64* handle) {

  AEEResult res;
  FARF(ALWAYS, "VVVVVVVVVVVVVVVVVVV Opening skel");

  android_app_handle* my_android_app_handle;
  my_android_app_handle = (android_app_handle*)malloc(sizeof(android_app_handle));
  my_android_app_handle->stats=(android_app_stats*)malloc(sizeof(android_app_stats));
  *my_android_app_handle->stats=(android_app_stats){0};

  worker_pool_context_t *worker_pool_context = (worker_pool_context_t*)malloc(sizeof(worker_pool_context_t));

  if(worker_pool_context == NULL)
  {
    FARF(ERROR, "Could not allocate memory for worker pool context");
    res = AEE_ENOMEMORY;
    goto bail;
  }

  int pid = qurt_process_get_id();
  FARF(ALWAYS, "Initializing worker pool with context %p on pid %x with open handle %p", worker_pool_context, pid, handle);
  res = worker_pool_init(worker_pool_context);
  if(res != AEE_SUCCESS)
  {
    free(worker_pool_context);
    FARF(ALWAYS, "Unable to create worker pool");
    goto bail;
  }

  // A way to determine at runtime whether we are on the simulator
  // This variable is needed to execute the HAP_compute_res_attr APIs conditionally
  // For the sake of code simplicity, we just do not have QTEST enabled for this
  // example and thus do not need to write code that can run on the simulator
  // on_simulator = ((0 != HAP_get_chip_family_id) && (HAP_get_chip_family_id()==0));

  my_android_app_handle->worker_pool_context = worker_pool_context;
  my_android_app_handle->terminate=0;
  *handle = (remote_handle64)my_android_app_handle;

  bail:

  HAP_setFARFRuntimeLoggingParams(0, NULL, 0);
  return res;
}

/**
 * @param handle, the value returned by open
 * @retval, 0 for success, should always succeed
 */
AEEResult android_app_close(remote_handle64 handle) {
  int pid = qurt_process_get_id();
  FARF(ALWAYS, "^^^^^^^^^^^^^ Closing skel");
  android_app_handle* my_android_app_handle = (android_app_handle*)handle;
  if(my_android_app_handle->worker_pool_context == NULL) {
    FARF(ERROR, "remote handle is NULL");
    return AEE_EINVHANDLE;
  }
  if(my_android_app_handle->stats == NULL) {
    FARF(ALWAYS, "stats pointer is NULL");
    return AEE_EINVHANDLE;
  }
  FARF(ALWAYS, "Deinitializing worker pool with context %p on pid %x with open handle %p",my_android_app_handle->worker_pool_context, pid, handle);
  worker_pool_deinit(my_android_app_handle->worker_pool_context);
  free(my_android_app_handle->worker_pool_context);
  free(my_android_app_handle->stats);
  free(my_android_app_handle);
  return AEE_SUCCESS;
}

AEEResult android_app_terminate(remote_handle64 handle) {
  FARF(RUNTIME_HIGH, "Requesting termination of ongoing task");
  android_app_handle* my_android_app_handle = (android_app_handle*)handle;
  my_android_app_handle->terminate=1;
  return AEE_SUCCESS;
}

AEEResult android_app_run(remote_handle64 handle, const android_app_configs* configs, const uint8* in_ptr, int in_length, uint8* out_ptr, int out_length) {

  HAP_setFARFRuntimeLoggingParams(configs->verbose_mask, NULL, 0);

  FARF(RUNTIME_HIGH,"Launched session %d on DSP", configs->session_id);

  uint64 start_time = HAP_perf_get_time_us();
  android_app_handle* my_android_app_handle = (android_app_handle*)handle;
  int has_task_been_interrupted=0;
  AEEResult returned_value;

  // Initialize workers
  worker_pool_job_t    job;
  worker_synctoken_t    token;
  worker_pool_synctoken_init(&token, configs->num_workers);
  job.fptr = android_app_worker_callback;
  android_app_worker_callback_t dptr = {0};
  job.dptr = (void *)&dptr;
  dptr.token = &token;
  dptr.configs = configs;
  dptr.start_time = start_time;
  // A simple sanity check on some parameters
  if ((in_length!=configs->array_width*configs->array_height) || (in_length!=out_length)) {
    FARF(ERROR, "Unexpected input parameters: %d * %d should equal %d and %d",configs->array_width, configs->array_height, in_length, out_length);
    return AEE_EBADPARM;
  }

  // Determine number of worker tasks and size of each task to ensure frequent-enough resource release
  HAP_power_response_t response;
  void *ctxt = (void*)handle;
  memset(&response, 0, sizeof(HAP_power_response_t));
  response.type = HAP_power_get_clk_Freq;
  returned_value = HAP_power_get(ctxt, &response);
  if(returned_value!=AEE_SUCCESS)    {
    FARF(ERROR, "Unable to get the DSP core clock frequency");
    dptr.is_task_failed=1;
    goto finalize_stats;
  }

  dptr.num_worker_task_cycles = (int)((unsigned long long)response.clkFreqHz * WORKER_TASK_DURATION_US / 1000000);
  int num_worker_tasks = (int)((unsigned long long)configs->mega_cycles_per_task*1000000/dptr.num_worker_task_cycles);
  dptr.num_worker_tasks=num_worker_tasks;
  int pid = qurt_process_get_id();
  if (pid) { // if (pid) just to remove compilation error due to unused variable
    FARF(RUNTIME_HIGH,
         "Session %d. DSP clk: %d. Requested MCycles / task= %d. Worker task cycles: %d. Num tasks: %d. PID=%d",
         configs->session_id, response.clkFreqHz, configs->mega_cycles_per_task,
         dptr.num_worker_task_cycles, num_worker_tasks, pid);
  }

  // Set the priority for this thread
  qurt_thread_set_priority(qurt_thread_get_id(), configs->thread_priority);

  compute_res_attr_t compute_res;
  if(configs->vtcm_size_mb>0){
      HAP_compute_res_attr_init(&compute_res);
      HAP_compute_res_attr_set_serialize(&compute_res, configs->is_serialized);
      HAP_compute_res_attr_set_vtcm_param(&compute_res, configs->vtcm_size_mb*1024, 1);
      HAP_compute_res_attr_set_release_callback(&compute_res, android_app_release_callback, (void *)&dptr);
  }

  while (1) {
    uint64 start_waiting_on_resource_time_us = HAP_perf_get_time_us();
    unsigned int context_id=0;
    FARF(RUNTIME_HIGH, "--- session %d requesting %d VTCM MB with a timeout of %d us",configs->session_id,configs->vtcm_size_mb, configs->resource_acquire_timeout_us);
    if(configs->vtcm_size_mb>0){
        context_id = HAP_compute_res_acquire(&compute_res, configs->resource_acquire_timeout_us);
        FARF(RUNTIME_HIGH, "--- session %d completed resource acquire",configs->session_id);
        if(context_id==0) {
            FARF(ERROR, "+++*** ERROR : session %d unable to acquire context after waiting for %d us ***", configs->session_id, (int)(HAP_perf_get_time_us() - start_waiting_on_resource_time_us));
            returned_value = AEE_ERESOURCENOTFOUND;
            dptr.is_task_failed=1;
            goto finalize_stats;
        }
    }
    else{
        FARF(RUNTIME_HIGH, "--- session %d bypassed resource acquire",configs->session_id);
    }
    my_android_app_handle->stats->waiting_on_resource_time_us+=(int)(HAP_perf_get_time_us() - start_waiting_on_resource_time_us);

    /* In this trivial example, we actually do not use VTCM, but a realistic example would share
     * or divide this memory among the workers. */
    if(configs->vtcm_size_mb>0){
        unsigned char *vtcm = (unsigned char *)HAP_compute_res_attr_get_vtcm_ptr(&compute_res);
        *vtcm=0;  // just a place holder to use that pointer
    }
    // Launch workers
    for (int i = 0; i < configs->num_workers; i++)    {
      /* It can be useful to serialize the workers for testing purpose. This can be accomplished
       * by executing the line commented out below instead of submitting tasks to the worker pool. */
      //job.fptr(job.dptr);
      (void) worker_pool_submit(*my_android_app_handle->worker_pool_context, job);
    }
    worker_pool_synctoken_wait(&token);
    if(configs->vtcm_size_mb>0){
        HAP_compute_res_release(context_id);
    }
#if DEBUG
    FARF(RUNTIME_HIGH, "session %d released resource",configs->session_id);
#endif
    if (!dptr.is_task_interrupted) {
      // If we reached this point without being interrupted, we know we are done
      break;
    }
    FARF(RUNTIME_HIGH, "session %d was interrupted before it could complete its task",configs->session_id);
    my_android_app_handle->stats->num_interruptions+=1;
    my_android_app_handle->stats->num_tasks_interrupted+=1-has_task_been_interrupted;
    has_task_been_interrupted=1;
    dptr.is_task_interrupted=0;
    // When an interruption occurs, we can't guarantee that any of the workers completed their task so we rewind the counter
    dptr.num_worker_tasks+=configs->num_workers;
    dptr.num_worker_tasks=dptr.num_worker_tasks>num_worker_tasks?num_worker_tasks:dptr.num_worker_tasks;
    worker_pool_synctoken_init(&token, configs->num_workers);
    FARF(RUNTIME_HIGH, "session %d num interruptions =  %d, num_interrupted=%d now resuming",configs->session_id,
            my_android_app_handle->stats->num_interruptions, my_android_app_handle->stats->num_tasks_interrupted);
  }

finalize_stats:

  if (dptr.is_task_aborted) {
    FARF(RUNTIME_HIGH, "Aborted time-critical task that could not complete on time");
    my_android_app_handle->stats->num_tasks_aborted++;
  } else if (dptr.is_task_failed) {
    FARF(RUNTIME_HIGH, "Failed task");
    my_android_app_handle->stats->num_tasks_failed++;
  } else {
    my_android_app_handle->stats->num_tasks_completed++;

    // A simple test to ensure we can manipulate arrays and to execute only when successful
    for (int i=0;i<in_length;i++) {
      *out_ptr++=  *in_ptr++;
    }
  }

  uint64 end_time = HAP_perf_get_time_us();
  my_android_app_handle->stats->dsp_time_us+=end_time - start_time;

#if DEBUG
  FARF(RUNTIME_HIGH,"Quick stats session id %d after one run: num tasks completed %d and failed %d - dsp time (ms) %d",
       configs->session_id, my_android_app_handle->stats->num_tasks_completed, my_android_app_handle->stats->num_tasks_failed,
       (int)my_android_app_handle->stats->dsp_time_us/1000);
#endif

  return 0;
}

AEEResult android_app_get_stats(remote_handle64 handle, android_app_stats* stats) {


    android_app_handle* my_android_app_handle = (android_app_handle*)handle;
  if(my_android_app_handle->stats == NULL) {
    FARF(ERROR, "stats pointer is NULL");
    return AEE_EINVHANDLE;
  }
  *stats = *my_android_app_handle->stats;
    FARF(RUNTIME_HIGH,"-------------- In get stats - dsp time (ms) %d",
          (int)stats->dsp_time_us/1000);
  return AEE_SUCCESS;
}

AEEResult android_app_dummy(remote_handle64 handle) {
  FARF(ALWAYS,"MY DUMMY FUNCTION");
  return 0;
}
