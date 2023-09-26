#ifndef _GPU_UTILS_H
#define _GPU_UTILS_H

// Sample Intel iGPU frequency and RCS busy

/**
Much of this code was taken, modified, and inpsired by Intel's gpu performance code.
https://gitlab.freedesktop.org/drm/igt-gpu-tools
**/

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <math.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <linux/perf_event.h>
#include <sys/sysinfo.h>

struct gpu_freq
{
	struct gpu_freq_stat
	{
		uint64_t act, rcs;
		uint64_t timestamp;
	} stat[2];
	int fd;
	int count;
	int rcs_busy;
	int current;
};

/**
 * Must be called once before or else any subsequent calls read_gpu_freq() will fail
 * This function initialzes data for reading gpu frequency.
 * @ Return int: 0 on success, error code on failure.
 */
int initialize_read_gpu_freq();

/**
 * @Return int: 0 if successful
 * freq_info will be filled with the current intel integrated GPU frequency in MHz and the RCS (Render Command Streamer) unit busy percentage during the last sampling interval
 */
int read_gpu_freq(int *freq_info);

#endif