#ifndef _MY_UTIL_H
#define _MY_UTIL_H

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <sys/sysinfo.h>

uint64_t get_time(void);

void pin_cpu(size_t core_ID);

void getMemory(int *currRealMem, int *peakRealMem, int *currVirtMem, int *peakVirtMem, int pid);

int _perf_open(uint64_t type, uint64_t config, int group, uint64_t format);

int sysfs_scanf(int dir, const char *attr, const char *fmt, ...);

#endif
