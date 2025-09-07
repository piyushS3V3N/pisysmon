#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "network.h"

// Function to parse the network statistics from /proc/net/dev
int parse_network_stats(struct NetworkStats *stats) {
    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp == NULL) {
        perror("Unable to open /proc/net/dev");
        return -1;
    }

    char line[256];
    int idx = 0;

    // Skip the first two lines (headers)
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    // Parse each line for the network stats
    while (fgets(line, sizeof(line), fp)) {
        if (idx >= INTERFACE_COUNT) break; // We only handle up to INTERFACE_COUNT interfaces

        // Parse interface name, RX and TX bytes (ignore other stats)
        sscanf(line, " %31s %llu %*d %*d %*d %*d %*d %*d %llu",
               stats[idx].interface_name,
               &stats[idx].rx_bytes,
               &stats[idx].tx_bytes);

        // Remove the colon from the interface name
        stats[idx].interface_name[strlen(stats[idx].interface_name) - 1] = '\0';

        idx++;
    }

    fclose(fp);
    return idx; // Return the number of interfaces found
}

// Function to calculate and display network usage in percentage
void get_network_usage(struct NetworkStats *prev_stats, struct NetworkStats *curr_stats, int count) {
    for (int i = 0; i < count; i++) {
        // Calculate the difference in received and transmitted bytes
        unsigned long long rx_diff = curr_stats[i].rx_bytes - prev_stats[i].rx_bytes;
        unsigned long long tx_diff = curr_stats[i].tx_bytes - prev_stats[i].tx_bytes;

        // Calculate the percentage (relative to the previous stats)
        double rx_percentage = (prev_stats[i].rx_bytes > 0) ? (double)rx_diff / prev_stats[i].rx_bytes * 100.0 : 0.0;
        double tx_percentage = (prev_stats[i].tx_bytes > 0) ? (double)tx_diff / prev_stats[i].tx_bytes * 100.0 : 0.0;

        // Display the interface stats
        printf("Interface: %s\n", curr_stats[i].interface_name);
        printf("  RX (bytes): %llu (%.2f%% increase)\n", rx_diff, rx_percentage);
        printf("  TX (bytes): %llu (%.2f%% increase)\n", tx_diff, tx_percentage);
    }
}
