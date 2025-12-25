# Parallel Digital Shredder

A high-performance, multithreaded file shredder that securely overwrites files using OpenMP parallel processing. This tool leverages modern CPU capabilities to perform fast, secure data destruction through concurrent operations.

## Overview

This digital file shredder provides secure file deletion by overwriting data with multiple passes using parallel processing. Built with C++17 and OpenMP, it efficiently utilizes multiple CPU cores to maximize throughput and minimize shredding time.

## Key Features

- **Multi-threaded Processing:** Utilizes OpenMP for efficient parallel execution across CPU cores
- **Configurable Overwrite Passes:** Support for multiple overwrite patterns (zeros, ones, random data)
- **Smart Chunk Distribution:** Automatically divides files into optimal chunks for parallel processing
- **Thread-Safe Operations:** Implements proper synchronization to ensure data integrity
- **Real-time Progress Tracking:** Displays thread status and performance metrics
- **Safety Confirmations:** Requires user confirmation before destructive operations

## Project Structure

```sh
digital_shredder/
├── main.cpp      # Main program with argument parsing and thread orchestration
├── shredder.h    # Core shredding logic and chunk processing
└── utils.cpp     # Utility functions for file validation and random generation
```

## Requirements

- **Compiler:** g++ with C++17 support
- **Libraries:** OpenMP (typically included with g++)
- **Platform:** Linux/Unix-based systems
- **Build Tools:** Make (optional)

## Compilation

### Linux/macOS

```bash
g++ -std=c++17 -Wall -Wextra -fopenmp -O2 main.cpp utils.cpp -o shredder
```

### Using Makefile

```bash
make
```

## Usage

```bash
./shredder <file_path> <passes> [threads]
```

### Parameters

- `file_path`: Target file to shred (required)
- `passes`: Number of overwrite passes, minimum 1 (required)
- `threads`: Number of OpenMP threads (optional, defaults to system maximum)

### Examples

```bash
# Shred a file with 3 passes using 4 threads
./shredder sensitive_data.txt 3 4

# Shred with 5 passes using all available CPU cores
./shredder document.pdf 5

# Shred with 7 passes using 8 threads
./shredder archive.zip 7 8
```

## How It Works

### Overwrite Algorithm

The shredder employs a multi-pass overwrite strategy to ensure data destruction:

1. **Pass 1:** Overwrites with zeros (`0x00`)
2. **Pass 2:** Overwrites with ones (`0xFF`)
3. **Pass 3:** Overwrites with random bytes

The pattern cycles for additional passes. Example with 7 passes:

- Pass 1: `0x00` → Pass 2: `0xFF` → Pass 3: Random → Pass 4: `0x00` → Pass 5: `0xFF` → Pass 6: Random → Pass 7: `0x00`

### Parallel Architecture

The file is divided into equal chunks, with each chunk assigned to a separate thread:

```sh
chunk_size = file_size / thread_count
remainder  = file_size % thread_count
```

The last thread handles any remainder bytes to ensure complete coverage.

### Implementation Details

- **Shared File Access:** All threads use the same file pointer with synchronized seeking
- **Thread-Private Buffers:** Each thread maintains its own 1MB write buffer
- **Non-Overlapping Writes:** Chunks are carefully calculated to prevent conflicts
- **Position Management:** Each thread calls `fseek()` before writing to ensure correct offset
- **Critical Sections:** Console output is synchronized to prevent garbled messages

## Performance Measurement

The program measures and reports:

- File size in bytes
- Number of threads used
- Number of overwrite passes
- Total execution time in milliseconds
- Throughput in MB/s

### Performance Characteristics

Parallel processing provides significant speedup, typically scaling near-linearly with core count:

| Threads | Expected Speedup |
|---------|------------------|
| 1       | Baseline         |
| 2       | 1.8-1.9x         |
| 4       | 3.5-3.8x         |
| 8       | 6-7x             |

**Performance factors:**

- Storage device write speed (I/O bottleneck)
- Number of physical CPU cores
- File size (larger files scale better)
- Number of overwrite passes

## Testing

### Create Test File

```bash
# Create a 10MB test file
dd if=/dev/urandom of=test_file.bin bs=1M count=10
```

### Run Shredder

```bash
./shredder test_file.bin 3 4
```

### Benchmark Sequential vs Parallel

```bash
# Sequential (1 thread)
./shredder test_file.bin 3 1

# Parallel (4 threads)
./shredder test_file.bin 3 4
```

Compare execution times to verify parallel speedup.

## Safety Features

The shredder includes multiple safety mechanisms:

1. **Visual Warning:** Displays a prominent warning banner before operation
2. **Confirmation Prompt:** Requires explicit user confirmation (`y`/`yes` to proceed, `n`/`no` to cancel)
3. **Pre-flight Validation:**
   - Verifies file existence
   - Confirms it's a regular file (not a directory or special file)
   - Checks write permissions
   - Validates non-zero file size
4. **Abort Capability:** User can safely cancel before any data modification occurs

## Technical Implementation

### OpenMP Features

| Feature | Purpose |
|---------|---------|
| `#pragma omp parallel for` | Distributes work across threads |
| `#pragma omp critical` | Synchronizes console output |
| `omp_set_num_threads()` | Configures thread pool size |
| `omp_get_max_threads()` | Detects system capabilities |

### Code Quality

- Zero global variables
- Clean, maintainable code structure
- Compiles cleanly with `-Wall -Wextra`
- Thread-safe by design
- Comprehensive error handling
- No memory leaks

## Important Warnings

⚠️ **Critical Information:**

1. **Irreversible Data Destruction:** This tool PERMANENTLY overwrites data. Files cannot be recovered after shredding.
2. **SSD Limitations:** Modern SSDs with wear-leveling may retain data fragments despite overwriting. For maximum security on SSDs, combine with full-disk encryption and consider physical destruction for highly sensitive data.
3. **Platform Compatibility:** Designed for Linux/Unix systems. Windows support would require modifications to file I/O operations.
4. **Use Responsibly:** Always verify the target file path before confirming the operation.

## Use Cases

- Secure deletion of sensitive documents
- Preparing storage devices for disposal
- Removing confidential data before system transfers
- Compliance with data destruction policies
- Testing parallel I/O performance

## Contributing

Contributions are welcome! Areas for improvement:

- Windows platform support
- Additional overwrite patterns (DoD 5220.22-M, Gutmann method)
- File system secure deletion after overwriting
- Progress bars and improved UI
- Batch file processing

## License

Open source software. Use at your own risk. The authors are not responsible for data loss or misuse.
