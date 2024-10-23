# Root Makefile for building and syncing both subprojects

# Directories
CLI_DIR = cli
COUNTER_DIR = counter
INSTALL_DIR = install

# Binaries
CLI_BINARY = $(CLI_DIR)/install/activity
COUNTER_BINARY = $(COUNTER_DIR)/install/counter

# Final install location
FINAL_INSTALL_DIR = $(INSTALL_DIR)

# Make commands for subprojects
.PHONY: all cli counter sync clean

# Build all projects and sync
all: cli counter sync

# Build cli project
cli:
	@echo "Building CLI project..."
	$(MAKE) -C $(CLI_DIR)

# Build counter project
counter:
	@echo "Building Counter project..."
	$(MAKE) -C $(COUNTER_DIR)

# Sync binaries to the common install directory
sync: $(CLI_BINARY) $(COUNTER_BINARY)
	@mkdir -p $(FINAL_INSTALL_DIR)
	@echo "Syncing binaries to $(FINAL_INSTALL_DIR)"
	rsync -av $(CLI_BINARY) $(COUNTER_BINARY) $(FINAL_INSTALL_DIR)

# Clean both projects
clean:
	@echo "Cleaning both subprojects..."
	$(MAKE) -C $(CLI_DIR) clean
	$(MAKE) -C $(COUNTER_DIR) clean
	@echo "Cleaning final install directory..."
	rm -rf $(FINAL_INSTALL_DIR)
