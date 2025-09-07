#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Unable to open /proc/stat");
        return -1;
    }

    char line[256];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    static long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0, prev_iowait = 0, prev_irq = 0, prev_softirq = 0, prev_steal = 0;

    long total_time, idle_time;
    long prev_total_time, prev_idle_time;

    while (fgets(line, sizeof(line), fp)) {
        // Check if line starts with "cpu" (either "cpu" or "cpuX")
        if (line[0] == 'c' && line[1] == 'p' && line[2] == 'u') {
            char cpu_name[10];

            // Parse CPU stats
            int ret = sscanf(line, "%s %ld %ld %ld %ld %ld %ld %ld %ld", cpu_name,
                              &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);

            if (ret != 9) {
                fprintf(stderr, "Error parsing line: %s\n", line);
                continue;
            }

            // Calculate total time and idle time
            total_time = user + nice + system + idle + iowait + irq + softirq + steal;
            idle_time = idle + iowait;

            // Calculate the change since the last read (using the static previous values)
            long total_diff = total_time - (prev_user + prev_nice + prev_system + prev_idle + prev_iowait + prev_irq + prev_softirq + prev_steal);
            long idle_diff = idle_time - (prev_idle + prev_iowait);

            if (total_diff > 0) {
                float cpu_usage = 100.0 * (1 - (float) idle_diff / total_diff);

                // Save the current values for the next calculation
                prev_user = user;
                prev_nice = nice;
                prev_system = system;
                prev_idle = idle;
                prev_iowait = iowait;
                prev_irq = irq;
                prev_softirq = softirq;
                prev_steal = steal;

                fclose(fp);

                return cpu_usage;
            }
        }
    }

    fclose(fp);
    return -1; // In case of failure
}
