#include "amd-gpu-utils.h"

#define SEARCH_DIR "/sys/class/hwmon"
#define DRIVER "amdgpu\n"
#define NAME "name"

static int fd = -1;

uint64_t amd_gpu_freq()
{

	// First time open the file
	if (fd < 0)
	{
		DIR *dir = opendir(SEARCH_DIR);
		if (dir == NULL)
		{
			perror("Error opening directory");
			return -1;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			// Look for the subdirectory whoes name = amdgpu
			if (entry->d_type == DT_DIR || entry->d_type == DT_LNK)
			{
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				{
					continue;
				}
				char filepath[PATH_MAX];
				snprintf(filepath, sizeof(filepath), "%s/%s/%s", SEARCH_DIR, entry->d_name, NAME);

				FILE *file = fopen(filepath, "r");
				if (file != NULL)
				{
					char content[256];
					if (fgets(content, sizeof(content), file) != NULL && strcmp(content, DRIVER) == 0)
					{
						fclose(file);
						snprintf(filepath, sizeof(filepath), "%s/%s/%s", SEARCH_DIR, entry->d_name, "freq1_input");
						fd = open(filepath, O_RDONLY);
						if (fd < 0)
						{
							return -1;
						}
						else
						{
							break;
						}
					}
					else
					{
						fclose(file);
					}
				}
			}
		}

		closedir(dir);
		if (fd < 0)
		{
			return -1;
		}
	}

	// Read the frequency
	char data[11];
	uint64_t frequency;
	int read_bytes = pread(fd, data, 11, 0);
	data[read_bytes] = '\0';

	sscanf(data, "%" SCNu64, &frequency);
	return frequency;
}

void amd_gpu_end()
{
	close(fd);
}
