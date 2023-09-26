#include "msr-utils.h"

// Inspired by multiple sources including
// https://github.com/intel/msr-tools/blob/master/cpuid.c
// https://github.com/intel/msr-tools/blob/master/rdmsr.c
uint64_t my_rdmsr_on_cpu(int core_ID, uint64_t reg)
{
	uint64_t data;
	static int msr_fd = -1;
	static int last_core_ID;

	if (msr_fd < 0 || last_core_ID != core_ID)
	{
		char msr_filename[BUFSIZ];
		if (msr_fd >= 0)
			close(msr_fd);
		sprintf(msr_filename, "/dev/cpu/%d/msr", core_ID);
		msr_fd = open(msr_filename, O_RDONLY);
		if (msr_fd < 0)
		{
			if (errno == ENXIO)
			{
				fprintf(stderr, "rdmsr: No CPU %d\n", core_ID);
				return -1;
			}
			else if (errno == EIO)
			{
				fprintf(stderr, "rdmsr: CPU %d doesn't support MSRs\n", core_ID);
				return -1;
			}
			else
			{
				perror("rdmsr: open");
				return -1;
			}
		}
		last_core_ID = core_ID;
	}

	if (pread(msr_fd, &data, sizeof data, reg) != sizeof data)
	{
		if (errno == EIO)
		{
			fprintf(stderr, "rdmsr: CPU %d cannot read MSR 0x%08" PRIx64 "\n", core_ID, reg);
			return -1;
		}
		else
		{
			perror("rdmsr: pread");
			return -1;
		}
	}

	return data;
}

int write_rdmsr_on_cpu(int core_ID, uint64_t reg, uint64_t value)
{
	static int msr_fd = -1;
	static int last_core_ID;

	if (msr_fd < 0 || last_core_ID != core_ID)
	{
		char msr_filename[BUFSIZ];
		if (msr_fd >= 0)
			close(msr_fd);
		sprintf(msr_filename, "/dev/cpu/%d/msr", core_ID);
		msr_fd = open(msr_filename, O_RDWR);
		if (msr_fd < 0)
		{
			if (errno == ENXIO)
			{
				fprintf(stderr, "wrmsr: No CPU %d\n", core_ID);
				return -1;
			}
			else if (errno == EIO)
			{
				fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n", core_ID);
				return -1;
			}
			else
			{
				perror("wrmsr: open");
				return -1;
			}
		}
		last_core_ID = core_ID;
	}

	if (pwrite(msr_fd, &value, sizeof value, reg) != sizeof value)
	{
		if (errno == EIO)
		{
			fprintf(stderr, "wrmsr: CPU %d cannot read MSR 0x%08" PRIx64 "\n", core_ID, reg);
			return -1;
		}
		else
		{
			perror("wrmsr: pwrite");
			return -1;
		}
	}

	return 0;
}