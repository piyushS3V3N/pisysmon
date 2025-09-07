#include <stdio.h>
#include <stdlib.h>
#include "memory_pi.h"

float get_memory_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Unable to open /proc/meminfo");
        return -1;
    }

    float mem_usage = 0;
    char line[256];
    long total_mem, free_mem, available_mem, buffers, cached;

    // Read each line of /proc/meminfo and extract relevant data
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &total_mem) == 1) {
            continue;
        }
        if (sscanf(line, "MemFree: %ld kB", &free_mem) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %ld kB", &available_mem) == 1) {
            continue;
        }
        if (sscanf(line, "Buffers: %ld kB", &buffers) == 1) {
            continue;
        }
        if (sscanf(line, "Cached: %ld kB", &cached) == 1) {
            continue;
        }
    }

    fclose(fp);

    // Calculate the total used memory (excluding buffers and cached memory)
    long used_memory = total_mem - free_mem - buffers - cached;

    // Calculate memory usage percentage
    mem_usage = ((float) used_memory / total_mem) * 100;

    // Print memory usage
   // printf("Memory Usage: %.2f%% (Used: %ld kB / Total: %ld kB)\n", mem_usage, used_memory, total_mem);

    return mem_usage;
}
