/*==============================================================================
  Copyright (c) 2021,2022 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#ifndef APP_RUN_H
#define APP_RUN_H

#include "AEEStdDef.h"
#include "android_app.h"

#ifdef __cplusplus
extern "C" {
#endif

int app_run(android_app_configs *c0, android_app_stats *s0, android_app_configs *c1,
            android_app_stats *s1, const char *cache_path);
int app_terminate();
int request_unsigned_pd();
#ifdef __cplusplus
}
#endif

#endif

