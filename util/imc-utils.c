#include "imc-utils.h"

static uint64_t __update_sample(struct pmu_counter *counter, uint64_t val, uint64_t curr_time)
{
	counter->val.prev = counter->val.curr;
	counter->val.curr = val;
	counter->val.prev_time = curr_time;
	return counter->val.curr - counter->val.prev;
}

static uint64_t update_sample(struct pmu_counter *counter, uint64_t *val, int index, uint64_t curr_time)
{
	return __update_sample(counter, val[index], curr_time);
}

// Read imc read and write from associated fd
static uint64_t pmu_read_multi(int fd, unsigned int num, uint64_t *val)
{
	uint64_t buf[2 + num];
	unsigned int i;
	ssize_t len;
	memset(buf, 0, sizeof(buf));
	len = read(fd, buf, sizeof(buf));
	assert(len == sizeof(buf));

	for (i = 0; i < num; i++)
	{
		val[i] = buf[2 + i];
	}
	return buf[1];
}

// Intel SKYLAKE 8700 IMC sampling

// Variable Declarations
static struct pmu_counter imc_reads, imc_writes;
static int imc_fd;

static int pmu_parse(struct pmu_counter *pmu, const char *path, const char *str)
{
	locale_t locale, oldlocale;
	bool result = true;
	char buf[128] = {};
	int dir;

	dir = open(path, O_RDONLY);
	if (dir < 0)
		return -1;

	/* Replace user environment with plain C to match kernel format */
	locale = newlocale(LC_ALL, "C", 0);
	oldlocale = uselocale(locale);

	result &= sysfs_scanf(dir, "type", "%" PRIu64, &pmu->type) == 1;

	snprintf(buf, sizeof(buf) - 1, "events/%s", str);
	result &= sysfs_scanf(dir, buf, "event=%" PRIx64, &pmu->config) == 1;

	snprintf(buf, sizeof(buf) - 1, "events/%s.scale", str);
	result &= sysfs_scanf(dir, buf, "%lf", &pmu->scale) == 1;

	snprintf(buf, sizeof(buf) - 1, "events/%s.unit", str);
	result &= sysfs_scanf(dir, buf, "%127s", buf) == 1;
	pmu->units = strdup(buf);

	uselocale(oldlocale);
	freelocale(locale);

	close(dir);

	if (!result)
		return -EINVAL;

	if (isnan(pmu->scale) || !pmu->scale)
		return -ERANGE;

	return 0;
}

static int imc_parse(struct pmu_counter *pmu, const char *str)
{
	return pmu_parse(pmu, "/sys/bus/event_source/devices/uncore_imc", str);
}

static int imc_open(struct pmu_counter *pmu,
					const char *domain)
{
	int fd;

	if (imc_parse(pmu, domain) < 0)
	{
		return -1;
	}
	fd = _perf_open(pmu->type, pmu->config, imc_fd, PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_GROUP);

	if (fd < 0)
		return -1;

	if (imc_fd == -1)
		imc_fd = fd;
	return 0;
}

static int imc_writes_open(struct pmu_counter *pmu)
{
	int ret = imc_open(pmu, "data_writes");
	return ret;
}

static int imc_reads_open(struct pmu_counter *pmu)
{
	int ret = imc_open(pmu, "data_reads");
	return ret;
}

// Initialize variables in PMU struct imc_reads and imc_writes
// Group Intel IMC events "imc_reads" and "imc_writes" into one perf event group
int imc_init()
{
	imc_fd = -1;
	int ret = imc_reads_open(&imc_reads);
	ret &= imc_writes_open(&imc_writes);
	imc_reads.val.prev_time = 0;
	imc_reads.val.prev = 0;
	imc_reads.val.curr = 0;
	imc_writes.val.prev_time = 0;
	imc_writes.val.prev = 0;
	imc_writes.val.curr = 0;
	return ret;
}

// Sample imc read&write within the last sampling interval
void imc_sample(double *rw)
{
	uint64_t val[2];
	uint64_t prev_time = imc_reads.val.prev_time;
	uint64_t curr_time = pmu_read_multi(imc_fd, 2, val);

	uint64_t read = update_sample(&imc_reads, val, 0, curr_time);
	uint64_t write = update_sample(&imc_writes, val, 1, curr_time);

	rw[0] = read * imc_reads.scale;
	rw[1] = write * imc_writes.scale;
}

// Intel ALDERLAKE 12700 IMC sampling

// Variable Declarations: there are two memory controllers
static struct pmu_counter imc_reads_1, imc_writes_1, imc_reads_2, imc_writes_2;
static int imc_alder_fd_1, imc_alder_fd_2;

static int imc_alder_parse(struct pmu_counter *pmu, const char *path, const char *str)
{
	locale_t locale, oldlocale;
	bool result = true;
	char buf[200] = {};
	int dir;

	dir = open(path, O_RDONLY);
	if (dir < 0)
		return -1;

	/* Replace user environment with plain C to match kernel format */
	locale = newlocale(LC_ALL, "C", 0);
	oldlocale = uselocale(locale);

	result &= sysfs_scanf(dir, "type", "%" PRIu64, &pmu->type) == 1;
	uint64_t mask;
	snprintf(buf, sizeof(buf) - 1, "events/%s", str);
	result &= sysfs_scanf(dir, buf, "event=0x%" PRIx64 ",umask=%" PRIx64, &pmu->config, &mask) == 2;
	pmu->config = ((uint32_t)pmu->config) | (((uint32_t)mask) << 8);

	snprintf(buf, sizeof(buf) - 1, "events/%s.scale", str);
	result &= sysfs_scanf(dir, buf, "%lf", &pmu->scale) == 1;

	snprintf(buf, sizeof(buf) - 1, "events/%s.unit", str);
	result &= sysfs_scanf(dir, buf, "%127s", buf) == 1;
	pmu->units = strdup(buf);

	uselocale(oldlocale);
	freelocale(locale);

	close(dir);
	if (!result)
		return -EINVAL;

	if (isnan(pmu->scale) || !pmu->scale)
		return -ERANGE;

	return 0;
}

static int imc_alder_open(int fd, struct pmu_counter *pmu, const char *directory, const char *domain)
{
	int curr_fd;

	if (imc_alder_parse(pmu, directory, domain) < 0)
	{
		return -1;
	}

	curr_fd = _perf_open(pmu->type, pmu->config, fd, PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_GROUP);

	if (curr_fd < 0)
		return -1;

	if (fd == -1)
		return curr_fd;

	return 0;
}

static int imc_writes_alder_open(int fd, struct pmu_counter *pmu, const char *directory)
{
	int ret = imc_alder_open(fd, pmu, directory, "data_write");
	return ret;
}

static int imc_reads_alder_open(int fd, struct pmu_counter *pmu, const char *directory)
{
	int ret = imc_alder_open(fd, pmu, directory, "data_read");
	return ret;
}

// Initialize variables in PMU struct imc_reads and imc_writes
// Each controller: Group Intel IMC events "imc_reads" and "imc_writes" into one perf event group
int imc_alder_init()
{
	imc_alder_fd_1 = -1;
	imc_alder_fd_2 = -1;

	imc_alder_fd_1 = imc_reads_alder_open(imc_alder_fd_1, &imc_reads_1, "/sys/bus/event_source/devices/uncore_imc_free_running_0");
	imc_writes_alder_open(imc_alder_fd_1, &imc_writes_1, "/sys/bus/event_source/devices/uncore_imc_free_running_0");
	imc_alder_fd_2 = imc_reads_alder_open(imc_alder_fd_2, &imc_reads_2, "/sys/bus/event_source/devices/uncore_imc_free_running_1");
	imc_writes_alder_open(imc_alder_fd_2, &imc_writes_2, "/sys/bus/event_source/devices/uncore_imc_free_running_1");

	imc_reads_1.val.prev_time = 0;
	imc_reads_1.val.prev = 0;
	imc_reads_1.val.curr = 0;
	imc_writes_1.val.prev_time = 0;
	imc_writes_1.val.prev = 0;
	imc_writes_1.val.curr = 0;
	imc_reads_2.val.prev_time = 0;
	imc_reads_2.val.prev = 0;
	imc_reads_2.val.curr = 0;
	imc_writes_2.val.prev_time = 0;
	imc_writes_2.val.prev = 0;
	imc_writes_2.val.curr = 0;

	return (imc_alder_fd_1 == -1) & (imc_alder_fd_2 == -1);
}

// Sample imc0 and imc1 read&write within the last sampling interval
// total = imc0+imc1
void imc_alder_sample(double *rw)
{
	uint64_t val[2];
	uint64_t prev_time = imc_reads_1.val.prev_time;
	uint64_t curr_time = pmu_read_multi(imc_alder_fd_1, 2, val);
	uint64_t read_1 = update_sample(&imc_reads_1, val, 0, curr_time);
	uint64_t write_1 = update_sample(&imc_writes_1, val, 1, curr_time);

	prev_time = imc_reads_2.val.prev_time;
	curr_time = pmu_read_multi(imc_alder_fd_2, 2, val);
	uint64_t read_2 = update_sample(&imc_reads_2, val, 0, curr_time);
	uint64_t write_2 = update_sample(&imc_writes_2, val, 1, curr_time);

	rw[0] = read_1 * imc_reads_1.scale + read_2 * imc_reads_2.scale;
	rw[1] = write_1 * imc_writes_1.scale + write_2 * imc_writes_2.scale;
}
