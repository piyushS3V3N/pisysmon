#ifndef NETWORK_H
#define NETWORK_H

// Define maximum interfaces count (adjust as needed)
#define INTERFACE_COUNT 5

// Structure for storing network stats
struct NetworkStats {
    char interface_name[32];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
};

// Function to parse network stats from /proc/net/dev
int parse_network_stats(struct NetworkStats *stats);

// Function to calculate and display network usage in percentage
void get_network_usage(struct NetworkStats *prev_stats, struct NetworkStats *curr_stats, int count);

#endif
