#include "gpu-utils.h"
#include <stdbool.h>
#include <locale.h>
#include "util.h"

/* GPU frequency and RCS busy*/

static struct gpu_freq gpu_freq_struct;

int gpu_freq_open_group(int group, const char *str)
{
	locale_t locale, oldlocale;

	// get type and config
	bool result = true;
	char buf[128] = {};
	int dir;

	dir = open("/sys/bus/event_source/devices/i915", O_RDONLY);
	if (dir < 0)
		return -1;

	/* Replace user environment with plain C to match kernel format */
	locale = newlocale(LC_ALL, "C", 0);
	oldlocale = uselocale(locale);

	uint64_t type;
	uint64_t event_config;

	result &= sysfs_scanf(dir, "type", "%" PRIu64, &type) == 1;

	snprintf(buf, sizeof(buf) - 1, "events/%s", str);
	result &= sysfs_scanf(dir, buf, "config=%" PRIx64, &event_config) == 1;

	uselocale(oldlocale);
	freelocale(locale);

	close(dir);
	if (!result)
		return -EINVAL;

	return _perf_open(type, event_config, group, PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_GROUP);
}

// Group Intel iGPU events "actual-frequency" and "rcs0-busy" into one perf event group
int gpu_perf_open(void)
{
	int fd;
	fd = gpu_freq_open_group(-1, "actual-frequency");
	if (gpu_freq_open_group(fd, "rcs0-busy") < 0)
	{
		close(fd);
		fd = -1;
	}
	return fd;
}

// Update "actual-frequency" and "rcs0-busy"
int gpu_freq_update(struct gpu_freq *gf)
{

	struct gpu_freq_stat *s = &gf->stat[gf->count++ & 1];
	struct gpu_freq_stat *d = &gf->stat[gf->count & 1];
	uint64_t data[4], d_time;
	int len;

	len = read(gf->fd, data, sizeof(data));
	if (len < 0)
		return -1;

	s->timestamp = data[1];
	s->act = data[2];
	s->rcs = data[3];

	d_time = s->timestamp - d->timestamp;
	if (d_time == 0)
	{
		gf->count--;
		return -1;
	}

	gf->current = (s->act - d->act) * 1e9 / d_time;
	double rcs_time = ((s->rcs - d->rcs) / d_time) * 100;
	gf->rcs_busy = (int)rcs_time;

	return 0;
}

// Initialization
int initialize_read_gpu_freq()
{
	memset(&gpu_freq_struct, 0, sizeof(gpu_freq_struct));
	gpu_freq_struct.fd = gpu_perf_open();
	if (gpu_freq_struct.fd < 0)
	{
		return -1;
	}
	return 0;
}

// Sample "actual-frequency" and "rcs0-busy"
int read_gpu_freq(int *freq_info)
{
	int error = gpu_freq_update(&gpu_freq_struct);
	if (error == 0)
	{
		freq_info[0] = gpu_freq_struct.current;
		freq_info[1] = gpu_freq_struct.rcs_busy;
		return 0;
	}
	else
	{
		fprintf(stderr, "gpu_freq_udpate failed, error code: %d\n", error);
		return -1;
	}
}
