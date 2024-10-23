# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Werror -Iinclude

# Executable target
TARGET = install/activity

# Object files
OBJS = build/cli.o build/cli_main.o

# Build the target executable
$(TARGET): $(OBJS)
	@mkdir -p install
	@echo "Linking $(OBJS) into $(TARGET)"
	$(CXX) $(OBJS) -o $(TARGET) -framework IOKit -framework CoreFoundation
	@echo "Executable created: $(TARGET)"

# Compile cli.cpp
build/cli.o: src/cli.cpp
	@mkdir -p build
	@echo "Compiling src/cli.cpp"
	$(CXX) $(CXXFLAGS) -c src/cli.cpp -o build/cli.o
	@echo "Object file created: build/cli.o"

# Compile cli_main.cpp
build/cli_main.o: src/cli_main.cpp
	@mkdir -p build
	@echo "Compiling src/cli_main.cpp"
	$(CXX) $(CXXFLAGS) -c src/cli_main.cpp -o build/cli_main.o
	@echo "Object file created: build/cli_main.o"

# Clean rule
.PHONY: clean
clean:
	@echo "Cleaning build directory and executable."
	rm -rf build install
	@echo "Cleaned build directory and executable."
