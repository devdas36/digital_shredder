# Parallel Digital Shredder Makefile

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -fopenmp -O2
TARGET = shredder
SOURCES = main.cpp utils.cpp
HEADERS = shredder.h

# Default target
all: $(TARGET)

# Build the shredder executable
$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)
	@echo "Build complete! Run with: ./$(TARGET) <file_path> <passes> [threads]"

# Clean build artifacts
clean:
	rm -f $(TARGET)
	rm -f test_file.bin benchmark_file.bin
	@echo "Clean complete!"

# Create a test file and run shredder
test: $(TARGET)
	@echo "Creating 10MB test file..."
	dd if=/dev/urandom of=test_file.bin bs=1M count=10 2>/dev/null
	@echo ""
	@echo "Running shredder with 3 passes and 4 threads..."
	./$(TARGET) test_file.bin 3 4

# Performance benchmark: compare single-threaded vs multi-threaded
benchmark: $(TARGET)
	@echo "=== Performance Benchmark ==="
	@echo ""
	@echo "Creating 50MB benchmark file..."
	dd if=/dev/urandom of=benchmark_file.bin bs=1M count=50 2>/dev/null
	@echo ""
	@echo "=== Test 1: Sequential (1 thread) ==="
	./$(TARGET) benchmark_file.bin 3 1
	@echo ""
	@echo "Recreating benchmark file..."
	dd if=/dev/urandom of=benchmark_file.bin bs=1M count=50 2>/dev/null
	@echo ""
	@echo "=== Test 2: Parallel (4 threads) ==="
	./$(TARGET) benchmark_file.bin 3 4
	@echo ""
	@echo "=== Benchmark Complete ==="

# Quick test with a small file
quick-test: $(TARGET)
	@echo "Creating 1MB test file..."
	dd if=/dev/urandom of=test_file.bin bs=1M count=1 2>/dev/null
	@echo ""
	./$(TARGET) test_file.bin 2 2

# Help target
help:
	@echo "Parallel Digital Shredder - Makefile Help"
	@echo "=========================================="
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build the shredder executable"
	@echo "  make all          - Same as 'make'"
	@echo "  make clean        - Remove build artifacts and test files"
	@echo "  make test         - Build and run with a 10MB test file"
	@echo "  make quick-test   - Build and run with a 1MB test file"
	@echo "  make benchmark    - Compare single vs multi-threaded performance"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Manual usage:"
	@echo "  ./$(TARGET) <file_path> <passes> [threads]"
	@echo ""

.PHONY: all clean test benchmark quick-test help
