
/*==============================================================================
  Copyright (c) 2012-2020-2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
  ==============================================================================*/

#include "android_app.h"
#include "android_app_async.h"
#include "remote.h"

#include "HAP_farf.h"
#undef FARF_HIGH
#define FARF_HIGH 1

AEEResult android_app_async_open(const char*uri, remote_handle64* handle) {
  FARF(ALWAYS, "VVVVVVVVVVVV Opening async skel");
  return android_app_open(uri, handle);
}

AEEResult android_app_async_dummy(remote_handle64 handle) {
  FARF(ALWAYS,"MY DUMMY FUNCTION");
  return 0;
}


AEEResult android_app_async_close(remote_handle64 handle) {
  FARF(ALWAYS, "^^^^^^^^^^^ Closing async skel");
  return android_app_close(handle);
}

AEEResult android_app_async_run(remote_handle64 handle, fastrpc_async_descriptor_t* async_desc, const android_app_configs* configs, const uint8* in_ptr, int in_length, uint8* out_ptr, int out_length) {
  FARF(ALWAYS, "Run async");
  return android_app_run(handle, configs, in_ptr, in_length, out_ptr, out_length);
}

AEEResult android_app_async_set_clocks(remote_handle64 h, int32 power_level, int32 latency, int32 dcvs_enabled) {
  FARF(ALWAYS, "^^^^^^^^^^^ Set clock async");
  return android_app_set_clocks(h,power_level,latency,dcvs_enabled);
}

AEEResult android_app_async_terminate(remote_handle64 h) {
  return android_app_terminate(h);
}

AEEResult android_app_async_get_stats(remote_handle64 handle, android_app_stats* stats) {
      return android_app_get_stats(handle,stats);
}
