#ifndef _AMD_GPU_UTILS_H
#define _AMD_GPU_UTILS_H

#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
/**
 * Read the AMD iGPU frequency from the hwmon interface exposed by the amdgpu driver
 */
uint64_t amd_gpu_freq();

void amd_gpu_end();

#endif