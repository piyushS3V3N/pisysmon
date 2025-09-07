# Pi System Monitor (pisysmon)

A dynamic, terminal-based system monitoring application designed specifically for linux system, built with ncurses for a responsive and interactive experience.

## Features

### Dynamic UI System
- **Responsive Layout**: Automatically adapts to terminal size changes
- **Real-time Resizing**: Handles terminal resize events gracefully
- **Modular Components**: Clean separation between UI rendering and data collection
- **Color-coded Display**: Different colors for CPU, Memory, Disk, and Network statistics

### System Monitoring
- **CPU Statistics**: Real-time CPU usage with detailed breakdowns
- **Memory Information**: Total, used, free, available, buffers, and cached memory
- **Disk Usage**: Multiple filesystem monitoring with usage percentages
- **Network Statistics**: Interface monitoring with data rates and packet counts

### Architecture Highlights
- **Clean Code Structure**: Modular design with separate concerns
- **Dynamic Memory Management**: Efficient resource handling
- **Error Handling**: Robust error checking and recovery
- **Cross-platform Compatibility**: POSIX-compliant implementation

## Installation

### Prerequisites
- GCC compiler
- ncurses development library
- Make build system

### On Debian/Ubuntu/Raspberry Pi OS:
```bash
sudo apt update
sudo apt install build-essential libncurses5-dev libncurses-dev
```

### Build from Source
```bash
# Clone or download the source code
cd pisysmon

# Compile the application
make

# Optional: Install system-wide
make install

# Optional: Build with debug symbols
make debug
```

## Usage

### Basic Usage
```bash
# Run with default 1-second update interval
./sysmon

# Run with custom update interval (2 seconds)
./sysmon -i 2

# Show help message
./sysmon --help
```

### Controls
- **q, Q, ESC**: Quit the application
- **Terminal resizing**: Automatically handled

### Command Line Options
- `-h, --help`: Display help message and exit
- `-i <seconds>`: Set update interval (1-60 seconds, default: 1)

## Architecture

### File Structure
```
pisysmon/
├── Makefile              # Build configuration
├── README.md            # This file
├── include/             # Header files
│   ├── cpu.h           # CPU monitoring interface
│   ├── disk.h          # Disk monitoring interface
│   ├── memory_pi.h     # Memory monitoring interface
│   ├── network.h       # Network monitoring interface
│   ├── sysmon.h        # System monitor core
│   └── ui.h            # UI management system
└── src/                # Source files
    ├── main.c          # Application entry point
    ├── cpu.c           # CPU statistics implementation
    ├── disk.c          # Disk usage implementation
    ├── memory_pi.c     # Memory statistics implementation
    ├── network.c       # Network statistics implementation
    ├── sysmon.c        # System monitor core implementation
    └── ui.c            # UI management implementation
```

### Key Components

#### UI System (`ui.h`, `ui.c`)
- **UILayout**: Manages overall screen layout and component positioning
- **UIComponent**: Individual display components (CPU, Memory, etc.)
- **Dynamic Sizing**: Calculates optimal component sizes based on terminal dimensions
- **Text Wrapping**: Intelligent text wrapping for content that exceeds component boundaries

#### System Monitor (`sysmon.h`, `sysmon.c`)
- **SystemMonitor**: Central data collection and management
- **Statistics Structures**: Typed data structures for each monitored subsystem
- **Update Management**: Coordinated updates of all system statistics
- **Data Formatting**: Utilities for human-readable data presentation

#### Individual Monitors
- **CPU Monitor**: Parses `/proc/stat` for CPU usage calculations
- **Memory Monitor**: Reads `/proc/meminfo` for memory statistics
- **Disk Monitor**: Uses `df` command for filesystem usage
- **Network Monitor**: Parses `/proc/net/dev` for network interface statistics

### Design Patterns

#### Component-Based Architecture
Each monitoring component is self-contained with clear interfaces:
```c
typedef struct {
    float usage_percent;
    long total_kb;
    long used_kb;
    // ... other fields
    bool valid;
} MemoryStats;
```

#### Observer Pattern
The UI system observes system statistics and updates displays accordingly.

#### Strategy Pattern
Different formatting strategies for various data types (bytes, rates, percentages).

## Key Improvements from Original Code

### Code Organization
- ✅ **Eliminated duplicate function declarations**
- ✅ **Separated UI logic from data collection**
- ✅ **Modular component architecture**
- ✅ **Clear separation of concerns**

### Dynamic UI Handling
- ✅ **Terminal resize detection and handling**
- ✅ **Dynamic component sizing and positioning**
- ✅ **Responsive layout calculations**
- ✅ **Intelligent text wrapping**

### Error Handling
- ✅ **Robust error checking throughout**
- ✅ **Graceful degradation on failures**
- ✅ **Input validation and sanitization**

### Performance
- ✅ **Efficient memory management**
- ✅ **Optimized rendering cycles**
- ✅ **Minimal system resource usage**

### Maintainability
- ✅ **Clear code structure and naming**
- ✅ **Comprehensive documentation**
- ✅ **Modular design for easy extension**

## Minimum Requirements

- **Terminal Size**: 80x24 characters minimum
- **System**: Linux-based system with `/proc` filesystem
- **Dependencies**: ncurses library
- **Memory**: Minimal (< 1MB runtime usage)

## Troubleshooting

### Common Issues

**Terminal too small error**
```
Error: Terminal too small. Minimum size is 80x24
```
Solution: Resize your terminal window or use a larger terminal.

**Permission denied for system files**
```
Unable to open /proc/stat
```
Solution: Ensure the application has read permissions for `/proc` filesystem files.

**Missing ncurses library**
```
error while loading shared libraries: libncurses.so.5
```
Solution: Install ncurses development packages as described in Prerequisites.

### Debug Mode
Build with debug symbols for troubleshooting:
```bash
make debug
gdb ./sysmon
```

## Future Enhancements

- [ ] Configuration file support
- [ ] Custom color themes
- [ ] Historical data graphs
- [ ] Process monitoring
- [ ] System alerts and notifications
- [ ] Export functionality
- [ ] Plugin architecture

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

Yet to be added

## Authors
Piyush Parashar (piyushS3V3N)
