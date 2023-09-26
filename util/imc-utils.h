#ifndef _IMC_UTILS_H
#define _IMC_UTILS_H
#include "util.h"
#include <stdbool.h>
#include <locale.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

// Sample Intel IMC data read and write perf events

/**
Much of this code was taken, modified, and inpsired by Intel's gpu performance code.
https://gitlab.freedesktop.org/drm/igt-gpu-tools/tools/intel_gpu_top.c
**/

struct pmu_pair
{
	uint64_t curr;		// curr imc read/write count
	uint64_t prev;		// previous imc read/write count
	uint64_t prev_time; // the last time we access imc counter
};

// struct for imc read/write pmu information
struct pmu_counter
{
	uint64_t type;
	uint64_t config;
	struct pmu_pair val;
	double scale;
	const char *units;
};

// Must be called before imc_sample
int imc_init();

void imc_sample(double *rw);

// Must be called before imc_alder_sample
int imc_alder_init();

void imc_alder_sample(double *rw);

#endif