#include "amd-df-utils.h"

static struct amd_imc_sample amd_imc_sample;

// Select the Data Fabric events to monitor
int amd_imc_init(int core_id)
{
    int r = write_rdmsr_on_cpu(core_id, MSR_DF_PERF_CTL_0, DRAM_CHANNEL_0_ALL);
    r |= write_rdmsr_on_cpu(core_id, MSR_DF_PERF_CTL_1, DRAM_CHANNEL_1_ALL);
    amd_imc_sample.prev = 0;
    amd_imc_sample.curr = 0;
    return r;
}

// Read the event counter
// The specific machine has one memory controller with two channels
uint64_t amd_imc_read(int core_id)
{
    uint64_t cha_0 = my_rdmsr_on_cpu(core_id, MSR_DF_PERF_CTR_0);
    uint64_t cha_1 = my_rdmsr_on_cpu(core_id, MSR_DF_PERF_CTR_1);
    uint64_t cha_all = cha_0 + cha_1;
    amd_imc_sample.prev = amd_imc_sample.curr;
    amd_imc_sample.curr = cha_all;
    return 64 * (amd_imc_sample.curr - amd_imc_sample.prev);
}