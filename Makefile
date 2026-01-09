CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lpthread

# Directories
SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

# Source files
CORE_SRCS = $(wildcard $(SRC_DIR)/core/*.c)
EVENT_SRCS = $(wildcard $(SRC_DIR)/event/*.c)
HTTP_SRCS = $(wildcard $(SRC_DIR)/http/*.c)
MODULE_SRCS = $(wildcard $(SRC_DIR)/modules/*.c)
CONFIG_SRCS = $(wildcard $(SRC_DIR)/config/*.c)

ALL_SRCS = $(CORE_SRCS) $(EVENT_SRCS) $(HTTP_SRCS) $(MODULE_SRCS) $(CONFIG_SRCS)
OBJS = $(ALL_SRCS:.c=.o)

# Target binary
TARGET = $(BUILD_DIR)/proton

# Default target - no optimization for Docker compatibility
all: CFLAGS += -O0 -g
all: $(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG -O0 -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: clean $(TARGET)

# Release build
release: CFLAGS += -O3 -DNDEBUG -march=native
release: clean $(TARGET)

# Build target
$(TARGET): $(OBJS) | $(BUILD_DIR)
	@echo "Linking $@"
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

# Compile source files
%.o: %.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts"
	@rm -f $(OBJS) $(TARGET)
	@rm -rf $(BUILD_DIR)/*.dSYM

# Install (requires root)
install: release
	@echo "Installing Proton Web Server"
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/proton
	install -d /etc/proton
	@if [ ! -f /etc/proton/proton.conf ]; then \
		install -m 644 conf/proton.conf.example /etc/proton/proton.conf; \
	fi
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling Proton Web Server"
	rm -f /usr/local/bin/proton
	@echo "Uninstallation complete (config files preserved)"

# Run with example config
run: all
	@echo "Starting Proton Web Server"
	$(TARGET) -c conf/proton.conf.example

# Run tests (placeholder)
test: debug
	@echo "Running tests..."
	@echo "No tests implemented yet"

# Format code
format:
	@echo "Formatting code"
	@find $(SRC_DIR) $(INCLUDE_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

# Show help
help:
	@echo "Proton Web Server - Makefile targets:"
	@echo "  make              - Build release version"
	@echo "  make all          - Build with optimizations"
	@echo "  make debug        - Build debug version with sanitizers"
	@echo "  make release      - Build optimized release version"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install      - Install to /usr/local/bin (requires root)"
	@echo "  make uninstall    - Remove installed binary"
	@echo "  make run          - Build and run with example config"
	@echo "  make test         - Run test suite"
	@echo "  make format       - Format source code with clang-format"
	@echo "  make help         - Show this help message"

.PHONY: all debug release clean install uninstall run test format help
