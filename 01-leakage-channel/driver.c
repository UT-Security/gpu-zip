#include <sys/resource.h>
#include <sys/types.h>
#include "../util/amd-gpu-utils.h"
#include "../util/amd-df-utils.h"
#include "../util/gpu-utils.h"
#include "../util/imc-utils.h"
#include "../util/util.h"

#define TIME_IMC 1000000L // 1 millisecond
#define TIME_GPU 5000000L // 5 millisecond
#define LEN 1000

static int rept_index = 0;
static int gpu_trace = 0;
static int imc_trace = 0;
volatile static int attacker_core_ID;

// Runs the given cpu_command
static void stress(void *cpu_command)
{
	system((char *)cpu_command);
}

struct args_t
{
	uint64_t iters;
	int selector;
};

// Collects the amount of data passes through the memory controller during each sampling interval TIME_IMC.
// Sample the memory usage of the target program "texture" every 1 second.
// On intel (i7-8700 and i7-12700), we rely on the IMC perf events.
// On AMD (Ryzen 7 4800U), we rely on the data fabric MSRs.
static __attribute__((noinline)) int monitor_imc(void *in)
{
	pin_cpu(attacker_core_ID);

	// Wait for 5 seconds
	sleep(5);

	struct args_t *arg = (struct args_t *)in;

	// Get the PID of texture program
	char line[LEN];
	FILE *cmd = popen("pidof texture", "r");
	fgets(line, LEN, cmd);
	char *pid;
	pid = strtok(line, " ");
	pclose(cmd);

	// Read the memory utilization of the texture program every 1 second
	FILE *mem_file;
	char mem_filename[200];
	sprintf(mem_filename, "./out/mem_%d_%06d.out", arg->selector, rept_index);
	mem_file = fopen((char *)mem_filename, "w");
	if (mem_file == NULL)
	{
		perror("Memory output file open fail");
		return 0;
	}

	FILE *imc_file;
	double rw[5];
	char imc_filename[200];
	sprintf(imc_filename, "./out/imc_%d_%06d.out", arg->selector, rept_index);
	imc_file = fopen((char *)imc_filename, "w");
	if (imc_file == NULL)
	{
		perror("IMC output file open fail");
		return 0;
	}

// Get initial sample for IMC PMU
#if ALDER
	imc_alder_sample(rw);
#elif AMD
	uint64_t amd_imc_data = amd_imc_read(attacker_core_ID);
#else
	imc_sample(rw);
#endif

	uint64_t time;
	// Collect measurements
	for (uint64_t i = 0; i < arg->iters; i++)
	{

		// Wait before next measurement
		nanosleep((const struct timespec[]){{0, TIME_IMC}}, NULL);
		time = get_time();
#if ALDER
		imc_alder_sample(rw);
		if (imc_trace == 1)
		{ // IMC read, IMC write
			fprintf(imc_file, "%.15f, %.15f, %" PRIu64 "\n", rw[0], rw[1], time);
		}
		else if (imc_trace == 2)
		{ // IMC read + IMC write
			fprintf(imc_file, "%.15f, %" PRIu64 "\n", rw[0] + rw[1], time);
		}
#elif AMD
		if (imc_trace == 2)
		{
			amd_imc_data = amd_imc_read(attacker_core_ID);
			double amd_imc_mib = (double)amd_imc_data / 1024.0 / 1024.0; // To MiB
			fprintf(imc_file, "%.15f, %" PRIu64 " \n", amd_imc_mib, time);
		}
#else
		imc_sample(rw);
		if (imc_trace == 1)
		{ // IMC read, IMC write
			fprintf(imc_file, "%.15f, %.15f, %" PRIu64 "\n", rw[0], rw[1], time);
		}
		else if (imc_trace == 2)
		{ // IMC read + IMC write
			fprintf(imc_file, "%.15f, %" PRIu64 "\n", rw[0] + rw[1], time);
		}
#endif

		if ((i % 1000) == 0)
		{
			int rm, prm, vm, pvm;
			getMemory(&rm, &prm, &vm, &pvm, atoi(pid));
			fprintf(mem_file, "%d, %d, %d, %d\n", rm, prm, vm, pvm);
		}
	}

	fclose(imc_file);
	fflush(mem_file);
	fclose(mem_file);

	return 0;
}

// AMD: collect the average iGPU frequency during each sampling interval TIME_GPU
// Intel: collect the average iGPU frequency and RCS-busy perf event counter update during each sampling interval TIME_GPU
static __attribute__((noinline)) int monitor_gpu(void *in)
{
	// Wait for 5 seconds
	sleep(5);

	struct args_t *arg = (struct args_t *)in;

	// Create the output file for iGPU trace
	FILE *gpu_file;
	int freq_info[2];
	char gpu_filename[200];
	sprintf(gpu_filename, "./out/gpu_%d_%06d.out", arg->selector, rept_index);
	gpu_file = fopen((char *)gpu_filename, "w");
	if (gpu_file == NULL)
	{
		perror("GPU output file open fail");
		return 0;
	}

// Get initial sample for iGPU PMUs
#if AMD
	uint64_t freq = amd_gpu_freq();
#else
	read_gpu_freq(freq_info);
#endif

	uint64_t total_run = arg->iters * TIME_IMC / TIME_GPU;
	uint64_t time;

	// Collect measurements
	for (uint64_t i = 0; i < total_run; i++)
	{
		// Wait before next measurement
		nanosleep((const struct timespec[]){{0, TIME_GPU}}, NULL);
		time = get_time();

#if AMD
		// Sample AMD GPU frequency
		freq = amd_gpu_freq();
		int freq_int = freq / 1000000;
		fprintf(gpu_file, "%d, %" PRIu64 " \n", freq_int, time);
#else
		// Sample GPU frequency and rcs0-busy
		int freq = read_gpu_freq(freq_info);
		// Actual frequency, rcs0-busy, current CPU cycle
		fprintf(gpu_file, "%d, %d, %" PRIu64 " \n", freq_info[0], freq_info[1], time);
#endif
	}

	fclose(gpu_file);

	return 0;
}

void read_selectors(char *filename, char **selectors, int *num_selectors)
{
	// Open the selector file
	FILE *selectors_file = fopen(filename, "r");
	if (selectors_file == NULL)
		perror("fopen error");

	// Read the selectors file line by line
	size_t len = 0;
	ssize_t read = 0;
	char *line = NULL;
	while ((read = getline(&line, &len, selectors_file)) != -1)
	{
		if (line[read - 1] == '\n')
			line[--read] = '\0';

		// Read selector
		selectors[*num_selectors] = strdup(line);
		*num_selectors += 1;
	}
}

int main(int argc, char *argv[])
{
	// Check arguments
	if (argc != 5)
	{
		fprintf(stderr, "Wrong Input! Enter: %s  <gpu> <imc> <samples> <outer>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Read in args
	struct args_t arg;
	int outer;

	// 1 if sample iGPU trace.
	sscanf(argv[1], "%d", &gpu_trace);

	// 1 if sample IMC trace as separate read and write
	// 2 if sample IMC trace as read and write combined
	sscanf(argv[2], "%d", &imc_trace);

	// Number of hardware samples
	sscanf(argv[3], "%" PRIu64, &(arg.iters));

	// Number of iterations that we loop through our selectors
	sscanf(argv[4], "%d", &outer);
	if (outer < 0)
	{
		fprintf(stderr, "outer cannot be negative!\n");
		exit(1);
	}

	int num_selectors = 0;
	char *selectors[1000];
	read_selectors("input.txt", selectors, &num_selectors);

	// Set the scheduling priority to high to avoid interruptions
	// (lower priorities cause more favorable scheduling, and -20 is the max)
	setpriority(PRIO_PROCESS, 0, -20);

	if (gpu_trace > 0)
	{
#if AMD
		// Sample the amdgpu frequency
		uint64_t amd_freq_read = amd_gpu_freq();
		if (amd_freq_read == -1)
		{
			perror("AMD frequency reading fail\n");
			exit(1);
		}
#else
		// Initialize IGPU frequency&rcs0-busy reading
		int freq_init = initialize_read_gpu_freq();
		if (freq_init)
		{
			perror("Intel GPU perf event open fail\n");
			exit(1);
		}
#endif
	}
	if (imc_trace > 0)
	{
// Initialize IMC PMU counters.
// Intel machine samples the IMC perf event. AMD samples the data fabric MSR counters.
#if ALDER
		int imc_init_ret = imc_alder_init();
		if (imc_init_ret)
		{
			perror("Intel ALDERLAKE IMC perf event open fail\n");
			exit(1);
		}
#elif AMD
		int amd_imc_init_ret = amd_imc_init(attacker_core_ID);
		if (amd_imc_init_ret != 0)
		{
			perror("AMD select data fabric perf event fail\n");
			exit(1);
		}
#else
		int imc_init_ret = imc_init();
		if (imc_init_ret)
		{
			perror("Intel SKYLAKE IMC perf event open fail\n");
			exit(1);
		}
#endif
	}

	// Run experiment once for each selector
	for (int i = 0; i < outer; i++)
	{
		for (int j = 0; j < num_selectors; j++)
		{

			// Set alternating selector, interleave the experiments to even out funny behaviours on the machine
			char *curr_command = selectors[j % num_selectors];
			arg.selector = j % num_selectors;

			// Prepare for experiments
			// thread for the opengl workload
			// thread_IMC for measuring IMC perf events
			// thread_GPU for measuring iGPU perf events
			pthread_t thread, thread_IMC, thread_GPU;

			char command[256];

			// Start opengl workload
			if (strlen(curr_command) != 0)
			{
				sprintf(command, "%s", curr_command);
				pthread_create(&thread, NULL, (void *)&stress, (void *)command);
			}
			if (imc_trace > 0)
			{
				pthread_create(&thread_IMC, NULL, (void *)&monitor_imc, (void *)&arg);
			}
			if (gpu_trace > 0)
			{
				pthread_create(&thread_GPU, NULL, (void *)&monitor_gpu, (void *)&arg);
			}

			if (imc_trace > 0)
			{
				pthread_join(thread_IMC, NULL);
			}
			if (gpu_trace > 0)
			{
				pthread_join(thread_GPU, NULL);
			}

			rept_index += 1;

			// Stop texture (our OpenGL workload always named "texture")
			system("pkill -f texture");

			// Join OpenGL workload texture
			if (strlen(curr_command) != 0)
			{
				pthread_join(thread, NULL);
			}
		}
	}

#if AMD
	if (gpu_trace > 0)
	{
		amd_gpu_end();
	}
#endif

	return 0;
}
