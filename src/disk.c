#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

void get_disk_usage() {
    FILE *fp = popen("df -h / 2>/dev/null", "r");
    if (fp == NULL) {
        return;
    }

    char line[256];
    int line_count = 0;
    
    // Skip header line and get filesystem info
    while (fgets(line, sizeof(line), fp) && line_count < 2) {
        line_count++;
        if (line_count == 2) {
            // This should be the root filesystem line
            char filesystem[64], size[16], used[16], avail[16], use_percent[8], mount[16];
            if (sscanf(line, "%63s %15s %15s %15s %7s %15s", 
                      filesystem, size, used, avail, use_percent, mount) == 6) {
                printf("Root filesystem (%s): %s used of %s (%s)\n", 
                       filesystem, used, size, use_percent);
            }
        }
    }

    pclose(fp);
}
