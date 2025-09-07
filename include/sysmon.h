#ifndef SYSMON_H
#define SYSMON_H

#include <stdbool.h>
#include <time.h>

// CPU Statistics structure
typedef struct {
    float usage_percent;
    long user_time;
    long system_time;
    long idle_time;
    long total_time;
    bool valid;
} CPUStats;

// Memory Statistics structure
typedef struct {
    long total_kb;
    long used_kb;
    long free_kb;
    long available_kb;
    long buffers_kb;
    long cached_kb;
    float usage_percent;
    bool valid;
} MemoryStats;

// Disk Statistics structure
typedef struct {
    char device[64];
    char mount_point[128];
    long total_kb;
    long used_kb;
    long available_kb;
    float usage_percent;
    bool valid;
} DiskStats;

// Network Statistics structure
typedef struct {
    char interface_name[32];
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    double rx_rate_mbps;
    double tx_rate_mbps;
    bool valid;
} NetworkStats;

// System Monitor main structure
typedef struct {
    CPUStats cpu;
    MemoryStats memory;
    DiskStats disks[8];
    int disk_count;
    NetworkStats interfaces[16];
    int interface_count;
    int update_interval_ms;
    bool running;
} SystemMonitor;

// Global system monitor instance
extern SystemMonitor g_sysmon;

// Initialization and cleanup
int sysmon_init(void);
void sysmon_cleanup(void);

// Update functions
void sysmon_update_all(void);
bool sysmon_update_cpu(CPUStats *cpu);
bool sysmon_update_memory(MemoryStats *memory);
int sysmon_update_disks(DiskStats *disks, int max_disks);
int sysmon_update_network(NetworkStats *interfaces, int max_interfaces);

// Utility functions
void sysmon_format_bytes(unsigned long long bytes, char *buffer, size_t buffer_size);
void sysmon_format_rate(double rate_mbps, char *buffer, size_t buffer_size);
const char* sysmon_get_error_string(void);

#endif // SYSMON_H
