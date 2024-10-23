# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Werror -Iinclude

# Executable target
TARGET = install/counter

# Object files
OBJS = build/counter.o build/counter_main.o

# Build the target executable
$(TARGET): $(OBJS)
	@mkdir -p install
	@echo "Linking $(OBJS) into $(TARGET)"
	$(CXX) $(OBJS) -o $(TARGET) -framework IOKit -framework CoreFoundation
	@echo "Executable created: $(TARGET)"

# Compile counter.cpp
build/counter.o: src/counter.cpp
	@mkdir -p build
	@echo "Compiling src/counter.cpp"
	$(CXX) $(CXXFLAGS) -c src/counter.cpp -o build/counter.o
	@echo "Object file created: build/counter.o"

# Compile counter_main.cpp
build/counter_main.o: src/counter_main.cpp
	@mkdir -p build
	@echo "Compiling src/counter_main.cpp"
	$(CXX) $(CXXFLAGS) -c src/counter_main.cpp -o build/counter_main.o
	@echo "Object file created: build/counter_main.o"

# Clean rule
.PHONY: clean
clean:
	@echo "Cleaning build directory and executable."
	rm -rf build install
	@echo "Cleaned build directory and executable."
