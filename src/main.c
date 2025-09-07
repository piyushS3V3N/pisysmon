#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>

#include "ui.h"
#include "sysmon.h"

// Application state
typedef struct {
    bool running;
    bool need_refresh;
} AppState;

static AppState app_state = {true, true};

// Signal handlers
void handle_sigint(int sig) {
    (void)sig;
    app_state.running = false;
}

void handle_sigwinch(int sig) {
    (void)sig;
    app_state.need_refresh = true;
    g_layout.layout_dirty = 1;
}

// Format CPU information for display
void format_cpu_info(char* buffer, size_t buffer_size) {
    CPUStats *cpu = &g_sysmon.cpu;

    if (!cpu->valid) {
        snprintf(buffer, buffer_size, "CPU information unavailable");
        return;
    }

    snprintf(buffer, buffer_size,
        "Usage: %.1f%%\n"
        "User Time: %ld\n"
        "System Time: %ld\n"
        "Idle Time: %ld\n"
        "Total Time: %ld",
        cpu->usage_percent,
        cpu->user_time,
        cpu->system_time,
        cpu->idle_time,
        cpu->total_time);
}

// Format memory information for display
void format_memory_info(char* buffer, size_t buffer_size) {
    MemoryStats *mem = &g_sysmon.memory;

    if (!mem->valid) {
        snprintf(buffer, buffer_size, "Memory information unavailable");
        return;
    }

    char total_str[32], used_str[32], free_str[32], avail_str[32];
    char buffers_str[32], cached_str[32];

    sysmon_format_bytes(mem->total_kb * 1024ULL, total_str, sizeof(total_str));
    sysmon_format_bytes(mem->used_kb * 1024ULL, used_str, sizeof(used_str));
    sysmon_format_bytes(mem->free_kb * 1024ULL, free_str, sizeof(free_str));
    sysmon_format_bytes(mem->available_kb * 1024ULL, avail_str, sizeof(avail_str));
    sysmon_format_bytes(mem->buffers_kb * 1024ULL, buffers_str, sizeof(buffers_str));
    sysmon_format_bytes(mem->cached_kb * 1024ULL, cached_str, sizeof(cached_str));

    snprintf(buffer, buffer_size,
        "Usage: %.1f%%\n"
        "Total: %s\n"
        "Used: %s\n"
        "Free: %s\n"
        "Available: %s\n"
        "Buffers: %s\n"
        "Cached: %s",
        mem->usage_percent,
        total_str,
        used_str,
        free_str,
        avail_str,
        buffers_str,
        cached_str);
}

// Format disk information for display
void format_disk_info(char* buffer, size_t buffer_size) {
    if (g_sysmon.disk_count == 0) {
        snprintf(buffer, buffer_size, "No disk information available");
        return;
    }

    char temp_buffer[2048] = {0};
    size_t offset = 0;

    for (int i = 0; i < g_sysmon.disk_count && i < 4; i++) {
        DiskStats *disk = &g_sysmon.disks[i];

        if (!disk->valid) continue;

        char total_str[32], used_str[32], avail_str[32];
        sysmon_format_bytes(disk->total_kb * 1024ULL, total_str, sizeof(total_str));
        sysmon_format_bytes(disk->used_kb * 1024ULL, used_str, sizeof(used_str));
        sysmon_format_bytes(disk->available_kb * 1024ULL, avail_str, sizeof(avail_str));

        int written = snprintf(temp_buffer + offset, sizeof(temp_buffer) - offset,
            "%s: %.1f%%\n"
            "  Mount: %s\n"
            "  Total: %s\n"
            "  Used: %s\n"
            "  Free: %s\n",
            disk->device,
            disk->usage_percent,
            disk->mount_point,
            total_str,
            used_str,
            avail_str);

        if (written > 0 && offset + written < sizeof(temp_buffer)) {
            offset += written;
        }

        if (i < g_sysmon.disk_count - 1 && i < 3) {
            int sep_written = snprintf(temp_buffer + offset, sizeof(temp_buffer) - offset, "\n");
            if (sep_written > 0 && offset + sep_written < sizeof(temp_buffer)) {
                offset += sep_written;
            }
        }
    }

    strncpy(buffer, temp_buffer, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

// Format network information for display
void format_network_info(char* buffer, size_t buffer_size) {
    if (g_sysmon.interface_count == 0) {
        snprintf(buffer, buffer_size, "No network interfaces found");
        return;
    }

    char temp_buffer[2048] = {0};
    size_t offset = 0;

    for (int i = 0; i < g_sysmon.interface_count && i < 4; i++) {
        NetworkStats *net = &g_sysmon.interfaces[i];

        if (!net->valid) continue;

        char rx_bytes_str[32], tx_bytes_str[32];
        char rx_rate_str[32], tx_rate_str[32];

        sysmon_format_bytes(net->rx_bytes, rx_bytes_str, sizeof(rx_bytes_str));
        sysmon_format_bytes(net->tx_bytes, tx_bytes_str, sizeof(tx_bytes_str));
        sysmon_format_rate(net->rx_rate_mbps, rx_rate_str, sizeof(rx_rate_str));
        sysmon_format_rate(net->tx_rate_mbps, tx_rate_str, sizeof(tx_rate_str));

        int written = snprintf(temp_buffer + offset, sizeof(temp_buffer) - offset,
            "%s:\n"
            "  RX: %s (%s)\n"
            "  TX: %s (%s)\n"
            "  Packets RX: %llu\n"
            "  Packets TX: %llu",
            net->interface_name,
            rx_bytes_str, rx_rate_str,
            tx_bytes_str, tx_rate_str,
            net->rx_packets,
            net->tx_packets);

        if (written > 0 && offset + written < sizeof(temp_buffer)) {
            offset += written;
        }

        if (i < g_sysmon.interface_count - 1 && i < 3) {
            int sep_written = snprintf(temp_buffer + offset, sizeof(temp_buffer) - offset, "\n\n");
            if (sep_written > 0 && offset + sep_written < sizeof(temp_buffer)) {
                offset += sep_written;
            }
        }
    }

    strncpy(buffer, temp_buffer, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
}

// Update all UI components with current system data
void update_display(void) {
    char buffer[2048];

    // Update CPU component
    format_cpu_info(buffer, sizeof(buffer));
    ui_update_component(COMPONENT_CPU, buffer);

    // Update Memory component
    format_memory_info(buffer, sizeof(buffer));
    ui_update_component(COMPONENT_MEMORY, buffer);

    // Update Disk component
    format_disk_info(buffer, sizeof(buffer));
    ui_update_component(COMPONENT_DISK, buffer);

    // Update Network component
    format_network_info(buffer, sizeof(buffer));
    ui_update_component(COMPONENT_NETWORK, buffer);
}

// Initialize all components
int initialize_components(void) {


    // Create UI components
    if (ui_create_component("CPU Statistics", COLOR_CPU) != COMPONENT_CPU) {
        return -1;
    }

    if (ui_create_component("Memory Statistics", COLOR_MEMORY) != COMPONENT_MEMORY) {
        return -1;
    }

    if (ui_create_component("Disk Usage", COLOR_DISK) != COMPONENT_DISK) {
        return -1;
    }

    if (ui_create_component("Network Statistics", COLOR_NETWORK) != COMPONENT_NETWORK) {
        return -1;
    }

    return 0;
}

// Main application loop
void main_loop(void) {
    time_t last_update = 0;
    time_t current_time;
    bool first_run = true;

    while (app_state.running) {
        current_time = time(NULL);

        // Handle resize events
        if (app_state.need_refresh || g_layout.layout_dirty) {
            ui_handle_resize();
            app_state.need_refresh = false;
            first_run = true; // Force redraw after resize
        }

        // Update data and display at specified intervals OR on first run
        if (first_run || (current_time - last_update) >= (g_sysmon.update_interval_ms / 1000)) {
            // Update system statistics
            sysmon_update_all();

            // Update display content (this will refresh individual components)
            update_display();

            // Refresh all windows
            ui_refresh_all();

            last_update = current_time;
            first_run = false;
        }

        // Check for user input without blocking
        int ch = getch();
        if (ch == 'q' || ch == 'Q' || ch == 27) { // ESC key
            app_state.running = false;
            break;
        }

        // Short sleep to prevent excessive CPU usage
        struct timespec sleep_time = {0, 100000000}; // 100ms
        nanosleep(&sleep_time, NULL);
    }
}

// Print usage information
void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -i <interval>  Update interval in seconds (default: 1)\n");
    printf("\nControls:\n");
    printf("  q, Q, ESC      Quit the application\n");
    printf("\nSystem Monitor made by PI\n");
}

// Parse command line arguments
int parse_arguments(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        } else if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                int interval = atoi(argv[i + 1]);
                if (interval > 0 && interval <= 60) {
                    g_sysmon.update_interval_ms = interval * 1000;
                    i++; // Skip the next argument
                } else {
                    fprintf(stderr, "Error: Invalid interval. Must be between 1 and 60 seconds.\n");
                    return -1;
                }
            } else {
                fprintf(stderr, "Error: -i option requires an argument.\n");
                return -1;
            }
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int arg_result = parse_arguments(argc, argv);
    if (arg_result != 0) {
        return (arg_result > 0) ? 0 : 1; // 0 for help, 1 for error
    }

    // Set up signal handlers
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    signal(SIGWINCH, handle_sigwinch);

    // Initialize system monitor
    if (sysmon_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize system monitor\n");
        return 1;
    }

    // Initialize UI
    if (ui_init() != 0) {
        fprintf(stderr, "Error: Failed to initialize user interface\n");
        sysmon_cleanup();
        return 1;
    }

    // Check terminal size
    if (g_layout.terminal_width < 80 || g_layout.terminal_height < 24) {
        ui_cleanup();
        sysmon_cleanup();
        fprintf(stderr, "Error: Terminal too small. Minimum size is 80x24, got %dx%d\n",
                g_layout.terminal_width, g_layout.terminal_height);
        return 1;
    }

    // Initialize components
    if (initialize_components() != 0) {
        fprintf(stderr, "Error: Failed to initialize UI components\n");
        ui_cleanup();
        sysmon_cleanup();
        return 1;
    }

    // Calculate initial layout - THIS WILL CREATE THE WINDOWS
    ui_calculate_layout();

    // Verify that windows were created successfully
    for (int i = 0; i < g_layout.num_components; i++) {
        if (g_layout.components[i].window == NULL) {
            fprintf(stderr, "Error: Failed to create window for component %d\n", i);
            ui_cleanup();
            sysmon_cleanup();
            return 1;
        }
    }

    // Initial data collection
    sysmon_update_all();

    // Run main application loop
    main_loop();

    // Cleanup
    ui_cleanup();
    sysmon_cleanup();

    printf("Pi System Monitor terminated.\n");
    return 0;
}
