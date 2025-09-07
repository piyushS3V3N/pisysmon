#define _POSIX_C_SOURCE 200809L

#include "sysmon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Global system monitor instance
SystemMonitor g_sysmon = {0};

// Static variables for CPU calculation
static long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0;
static long prev_iowait = 0, prev_irq = 0, prev_softirq = 0, prev_steal = 0;
static bool cpu_initialized = false;

// Static variables for network rate calculation
static NetworkStats prev_network_stats[16];
static time_t prev_network_time = 0;
static bool network_initialized = false;

int sysmon_init(void) {
    memset(&g_sysmon, 0, sizeof(SystemMonitor));
    g_sysmon.update_interval_ms = 1000;
    g_sysmon.running = true;
    
    // Initialize previous stats
    cpu_initialized = false;
    network_initialized = false;
    prev_network_time = 0;
    
    return 0;
}

void sysmon_cleanup(void) {
    g_sysmon.running = false;
}

void sysmon_update_all(void) {
    if (!g_sysmon.running) return;
    
    // Update all system statistics
    sysmon_update_cpu(&g_sysmon.cpu);
    sysmon_update_memory(&g_sysmon.memory);
    g_sysmon.disk_count = sysmon_update_disks(g_sysmon.disks, 8);
    g_sysmon.interface_count = sysmon_update_network(g_sysmon.interfaces, 16);
}

bool sysmon_update_cpu(CPUStats *cpu) {
    if (!cpu) return false;
    
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        cpu->valid = false;
        return false;
    }
    
    char line[256];
    long user, nice, system, idle, iowait, irq, softirq, steal;
    
    // Read the first line (overall CPU stats)
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        cpu->valid = false;
        return false;
    }
    
    // Parse CPU stats
    int ret = sscanf(line, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
                     &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    
    fclose(fp);
    
    if (ret != 8) {
        cpu->valid = false;
        return false;
    }
    
    // Calculate current totals
    long total_time = user + nice + system + idle + iowait + irq + softirq + steal;
    long idle_time = idle + iowait;
    
    cpu->user_time = user;
    cpu->system_time = system;
    cpu->idle_time = idle;
    cpu->total_time = total_time;
    
    // Calculate usage percentage if we have previous data
    if (cpu_initialized) {
        long prev_total = prev_user + prev_nice + prev_system + prev_idle + prev_iowait + prev_irq + prev_softirq + prev_steal;
        long prev_idle_total = prev_idle + prev_iowait;
        
        long total_diff = total_time - prev_total;
        long idle_diff = idle_time - prev_idle_total;
        
        if (total_diff > 0) {
            cpu->usage_percent = 100.0 * (1.0 - (float)idle_diff / total_diff);
        } else {
            cpu->usage_percent = 0.0;
        }
    } else {
        cpu->usage_percent = 0.0;
        cpu_initialized = true;
    }
    
    // Save current values for next calculation
    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;
    
    cpu->valid = true;
    return true;
}

bool sysmon_update_memory(MemoryStats *memory) {
    if (!memory) return false;
    
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        memory->valid = false;
        return false;
    }
    
    char line[256];
    long total = 0, free = 0, available = 0, buffers = 0, cached = 0;
    
    // Parse memory information
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %ld kB", &total) == 1) {
            continue;
        }
        if (sscanf(line, "MemFree: %ld kB", &free) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %ld kB", &available) == 1) {
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
    
    if (total <= 0) {
        memory->valid = false;
        return false;
    }
    
    // Calculate memory statistics
    memory->total_kb = total;
    memory->free_kb = free;
    memory->available_kb = available;
    memory->buffers_kb = buffers;
    memory->cached_kb = cached;
    memory->used_kb = total - free - buffers - cached;
    memory->usage_percent = (float)memory->used_kb / total * 100.0;
    memory->valid = true;
    
    return true;
}

int sysmon_update_disks(DiskStats *disks, int max_disks) {
    if (!disks || max_disks <= 0) return 0;
    
    FILE *fp = popen("df -k 2>/dev/null", "r");
    if (!fp) return 0;
    
    char line[512];
    int count = 0;
    
    // Skip header line
    if (fgets(line, sizeof(line), fp) == NULL) {
        pclose(fp);
        return 0;
    }
    
    // Parse each filesystem
    while (fgets(line, sizeof(line), fp) && count < max_disks) {
        char device[64], mount_point[128];
        long total, used, available;
        int usage_percent;
        
        // Parse the df output
        int ret = sscanf(line, "%63s %ld %ld %ld %d%% %127s",
                         device, &total, &used, &available, &usage_percent, mount_point);
        
        if (ret == 6) {
            // Skip special filesystems
            if (strncmp(device, "/dev/", 5) != 0 && 
                strncmp(device, "tmpfs", 5) != 0 &&
                strncmp(device, "udev", 4) != 0) {
                continue;
            }
            
            // Fill disk stats
            strncpy(disks[count].device, device, sizeof(disks[count].device) - 1);
            disks[count].device[sizeof(disks[count].device) - 1] = '\0';
            
            strncpy(disks[count].mount_point, mount_point, sizeof(disks[count].mount_point) - 1);
            disks[count].mount_point[sizeof(disks[count].mount_point) - 1] = '\0';
            
            disks[count].total_kb = total;
            disks[count].used_kb = used;
            disks[count].available_kb = available;
            disks[count].usage_percent = usage_percent;
            disks[count].valid = true;
            
            count++;
        }
    }
    
    pclose(fp);
    return count;
}

int sysmon_update_network(NetworkStats *interfaces, int max_interfaces) {
    if (!interfaces || max_interfaces <= 0) return 0;
    
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return 0;
    
    char line[256];
    int count = 0;
    time_t current_time = time(NULL);
    double time_diff = network_initialized ? (double)(current_time - prev_network_time) : 1.0;
    
    // Skip first two header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    
    // Parse network interface data
    while (fgets(line, sizeof(line), fp) && count < max_interfaces) {
        char interface_name[32];
        unsigned long long rx_bytes, tx_bytes, rx_packets, tx_packets;
        unsigned long long dummy;
        
        // Parse interface line
        int ret = sscanf(line, " %31s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                         interface_name, &rx_bytes, &rx_packets, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy,
                         &tx_bytes, &tx_packets, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy);
        
        if (ret >= 10) {
            // Remove colon from interface name
            char *colon = strchr(interface_name, ':');
            if (colon) *colon = '\0';
            
            // Skip loopback interface
            if (strcmp(interface_name, "lo") == 0) continue;
            
            // Fill interface stats
            strncpy(interfaces[count].interface_name, interface_name, sizeof(interfaces[count].interface_name) - 1);
            interfaces[count].interface_name[sizeof(interfaces[count].interface_name) - 1] = '\0';
            
            interfaces[count].rx_bytes = rx_bytes;
            interfaces[count].tx_bytes = tx_bytes;
            interfaces[count].rx_packets = rx_packets;
            interfaces[count].tx_packets = tx_packets;
            
            // Calculate rates if we have previous data
            if (network_initialized && time_diff > 0) {
                // Find previous stats for this interface
                NetworkStats *prev = NULL;
                for (int i = 0; i < 16; i++) {
                    if (strcmp(prev_network_stats[i].interface_name, interface_name) == 0) {
                        prev = &prev_network_stats[i];
                        break;
                    }
                }
                
                if (prev) {
                    // Calculate byte rate in Mbps
                    unsigned long long rx_diff = rx_bytes - prev->rx_bytes;
                    unsigned long long tx_diff = tx_bytes - prev->tx_bytes;
                    
                    interfaces[count].rx_rate_mbps = (double)rx_diff / time_diff / 1024.0 / 1024.0 * 8.0;
                    interfaces[count].tx_rate_mbps = (double)tx_diff / time_diff / 1024.0 / 1024.0 * 8.0;
                } else {
                    interfaces[count].rx_rate_mbps = 0.0;
                    interfaces[count].tx_rate_mbps = 0.0;
                }
            } else {
                interfaces[count].rx_rate_mbps = 0.0;
                interfaces[count].tx_rate_mbps = 0.0;
            }
            
            interfaces[count].valid = true;
            count++;
        }
    }
    
    fclose(fp);
    
    // Save current stats for next calculation
    memcpy(prev_network_stats, interfaces, sizeof(NetworkStats) * count);
    prev_network_time = current_time;
    network_initialized = true;
    
    return count;
}

void sysmon_format_bytes(unsigned long long bytes, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = (double)bytes;
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    if (unit_index == 0) {
        snprintf(buffer, buffer_size, "%llu %s", bytes, units[unit_index]);
    } else {
        snprintf(buffer, buffer_size, "%.1f %s", size, units[unit_index]);
    }
}

void sysmon_format_rate(double rate_mbps, char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    if (rate_mbps < 1.0) {
        snprintf(buffer, buffer_size, "%.1f Kbps", rate_mbps * 1024.0);
    } else if (rate_mbps < 1024.0) {
        snprintf(buffer, buffer_size, "%.1f Mbps", rate_mbps);
    } else {
        snprintf(buffer, buffer_size, "%.1f Gbps", rate_mbps / 1024.0);
    }
}

const char* sysmon_get_error_string(void) {
    return "System monitoring error";
}