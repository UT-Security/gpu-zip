#ifndef _MSR_UTILS_H
#define _MSR_UTILS_H

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <x86intrin.h>

uint64_t my_rdmsr_on_cpu(int core_ID, uint64_t reg);
int write_rdmsr_on_cpu(int core_ID, uint64_t reg, uint64_t value);

#endif