#ifndef _AMD_DF_UTILS_H
#define _AMD_DF_UTILS_H
#include "msr-utils.h"
#include <unistd.h>
#include <inttypes.h>

// Inspired by https://elixir.bootlin.com/linux/v6.2/source/tools/perf/pmu-events/arch/x86/amdzen2/data-fabric.json
// 			   https://github.com/clamchowder/MsrUtil/blob/2c3d7601368adca0c5f2cb43f17b0e6d92309a8d/AMD/Zen2DataFabric.cs

// The Performance Event Select
#define MSR_DF_PERF_CTL_0 0xC0010240
#define MSR_DF_PERF_CTL_1 0xC0010242
#define MSR_DF_PERF_CTL_2 0xC0010244
#define MSR_DF_PERF_CTL_3 0xC0010246

// Data Fabric Performance Event Counter
#define MSR_DF_PERF_CTR_0 0xC0010241
#define MSR_DF_PERF_CTR_1 0xC0010243
#define MSR_DF_PERF_CTR_2 0xC0010245
#define MSR_DF_PERF_CTR_3 0xC0010247

// Data fabric performance event MSR
#define DRAM_CHANNEL_0_ALL 0x403807 // "DRAM Channel Controller Request Types: Requests with Data (64B): DRAM Channel Controller 0"
#define DRAM_CHANNEL_1_ALL 0x403847 // "DRAM Channel Controller Request Types: Requests with Data (64B): DRAM Channel Controller 1"

struct amd_imc_sample
{
	uint64_t prev;
	uint64_t curr;
};

int amd_imc_init(int core_id);

uint64_t amd_imc_read(int core_id);

#endif
