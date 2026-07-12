// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200809L
#include <mcu/core.h>

#include <errno.h>

#include "timing.h"

void clock_start(struct timespec* timer) {
   if (clock_gettime(CLOCK_MONOTONIC, timer) != 0) {
      panic("Failed to fetch time, reason: \"%s\"", strerror(errno));
   }
}

double clock_end(struct timespec* timer) {
   struct timespec end;

   if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
      panic("Failed to fetch time, reason: \"%s\"", strerror(errno));
   }

   return
      (double) (end.tv_sec - timer->tv_sec) +
      (double) (end.tv_nsec - timer->tv_nsec) * 1e-9;
}
