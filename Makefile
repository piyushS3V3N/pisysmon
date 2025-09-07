CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -std=c99
LDFLAGS=-lncurses
SOURCES=src/main.c src/cpu.c src/memory_pi.c src/disk.c src/network.c src/ui.c src/sysmon.c
OBJECTS=$(SOURCES:.c=.o)
TARGET=pisysmon

# Default target
all: $(TARGET)

# Link object files into final executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Compile .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(TARGET) $(OBJECTS)

# Install target (optional)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)

# Uninstall target (optional)
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build the system monitor (default)"
	@echo "  clean     - Remove compiled files"
	@echo "  debug     - Build with debug symbols"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help message"

.PHONY: all clean install uninstall debug help
