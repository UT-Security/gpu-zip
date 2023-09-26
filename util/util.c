#include "util.h"
#include <stdarg.h>

/*
 * Gets the value Time Stamp Counter
 */
uint64_t get_time(void)
{
	uint64_t cycles;
	asm volatile("rdtscp\n\t"
				 "shl $32, %%rdx\n\t"
				 "or %%rdx, %0\n\t"
				 : "=a"(cycles)
				 :
				 : "rcx", "rdx", "memory");

	return cycles;
}

/*
 * Pin thread to CPU core_ID
 */
void pin_cpu(size_t core_ID)
{
	cpu_set_t set;
	CPU_ZERO(&set);
	CPU_SET(core_ID, &set);
	if (sched_setaffinity(0, sizeof(cpu_set_t), &set) < 0)
	{
		printf("Unable to Set Affinity\n");
		exit(EXIT_FAILURE);
	}
}

/*
 * Inspired by: https://stackoverflow.com/questions/1558402/memory-usage-of-current-process-in-c
 * Measures the current (and peak) resident and virtual memories
 * Usage of linux C process, in kB
 * Difference between vm and rs: https://www.baeldung.com/linux/resident-set-vs-virtual-memory-size#:~:text=This%20is%20a%20measure%20of,all%20heap%20and%20stack%20memory.
 */
void getMemory(
	int *currRealMem, int *peakRealMem,
	int *currVirtMem, int *peakVirtMem, int pid)
{

	// stores each word in status file
	char buffer[1024] = "";

	// linux file contains this-process info
	char fn[100];
	snprintf(fn, sizeof(fn), "/proc/%d/status", pid);
	FILE *file = fopen(fn, "r");
	if (file)
	{
		// read the entire file
		while (fscanf(file, " %1023s", buffer) == 1)
		{

			if (strcmp(buffer, "VmRSS:") == 0)
			{
				fscanf(file, " %d", currRealMem);
			}
			if (strcmp(buffer, "VmHWM:") == 0)
			{
				fscanf(file, " %d", peakRealMem);
			}
			if (strcmp(buffer, "VmSize:") == 0)
			{
				fscanf(file, " %d", currVirtMem);
			}
			if (strcmp(buffer, "VmPeak:") == 0)
			{
				fscanf(file, " %d", peakVirtMem);
			}
		}
		fclose(file);
	}
}

int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
#ifndef __NR_perf_event_open
#if defined(__i386__)
#define __NR_perf_event_open 336
#elif defined(__x86_64__)
#define __NR_perf_event_open 298
#else
#define __NR_perf_event_open 0
#endif
#endif
	attr->size = sizeof(*attr);
	return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

// Set attribute and do the perf_event_open sys call
// Inspired by https://gitlab.freedesktop.org/drm/igt-gpu-tools/
int _perf_open(uint64_t type, uint64_t config, int group, uint64_t format)
{
	struct perf_event_attr attr = {};
	int nr_cpus = get_nprocs_conf();
	static int cpu = 0;
	int ret = -1;

	attr.type = type;
	if (attr.type == 0)
		return -ENOENT;

	if (group >= 0)
		format &= ~PERF_FORMAT_GROUP;

	attr.read_format = format;
	attr.config = config;
	attr.use_clockid = 1;
	attr.clockid = CLOCK_MONOTONIC;

	do
	{
		cpu = (cpu + 1) % nr_cpus;
		ret = perf_event_open(&attr, -1, cpu, group, 0);
	} while ((ret < 0 && errno == EINVAL) && (cpu < nr_cpus));

	return ret;
}

// Inspired by https://gitlab.freedesktop.org/drm/igt-gpu-tools/-/blob/master/lib/igt_sysfs.c
/**
 * igt_sysfs_scanf:
 * @dir: directory for the device from igt_sysfs_open()
 * @attr: name of the sysfs node to open
 * @fmt: scanf format string
 * @...: Additional paramaters to store the scaned input values
 *
 * scanf() wrapper for sysfs.
 *
 * Returns:
 * Number of values successfully scanned (which can be 0), EOF on errors or
 * premature end of file.
 */
__attribute__((format(scanf, 3, 4))) int sysfs_scanf(int dir, const char *attr, const char *fmt, ...)
{
	FILE *file;
	int fd;
	int ret = -1;

	fd = openat(dir, attr, O_RDONLY);
	if (fd < 0)
		return -1;

	file = fdopen(fd, "r");
	if (file)
	{
		va_list ap;

		va_start(ap, fmt);
		ret = vfscanf(file, fmt, ap);
		va_end(ap);

		fclose(file);
	}
	else
	{
		close(fd);
	}
	return ret;
}