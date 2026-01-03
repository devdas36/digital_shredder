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
- **Optional Deletion:** User decides whether to delete the file after shredding
- **SSD Detection:** Automatically detects SSD storage devices (Windows & Linux)
- **TRIM Support:** Issues TRIM commands on SSDs to properly free space and minimize forensic traces
- **Cross-Platform:** Works on both Windows and Linux operating systems

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
- **Platform:** Windows or Linux
- **Build Tools:** Make (optional)

## Compilation

### Windows

```bash
g++ -std=c++17 -Wall -Wextra -fopenmp -O2 main.cpp utils.cpp -o shredder.exe
```

### Linux

```bash
g++ -std=c++17 -Wall -Wextra -fopenmp -O2 main.cpp utils.cpp -o shredder
```

### Using Makefile

```bash
make
```

## Usage

```bash
# Windows
./shredder.exe <file_path> <passes> [threads]

# Linux
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
4. **Optional Deletion:** After shredding, user chooses whether to delete the file or keep it
5. **Abort Capability:** User can safely cancel before any data modification occurs

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

2. **SSD Wear-Leveling:** Modern SSDs use wear-leveling which redirects writes to different physical locations. This tool addresses this by:
   - Detecting SSD storage devices automatically (both Windows and Linux)
   - Issuing TRIM commands after overwriting to mark space as unused
   - Allowing the SSD controller to properly deallocate blocks
   - Minimizing forensic traces by freeing the space
   
   **SSD Limitations:** Even with TRIM support, SSDs have inherent limitations:
   - **Firmware Control:** The SSD's firmware decides when to actually erase physical blocks
   - **Over-Provisioning:** SSDs maintain extra hidden space; data may persist there
   - **Reserved Blocks:** Some blocks are reserved for wear-leveling and may contain old data
   - **TRIM Timing:** The physical erasure happens asynchronously, not immediately
   
   **Maximum Security for SSDs:**
   - Use hardware-based secure erase commands (ATA Secure Erase)
   - Enable full-disk encryption before storing sensitive data
   - Consider physical destruction for mission-critical security
   
3. **Optional File Deletion:** After shredding, you choose whether to delete the file:
   - **If deleted:** Disk space is freed and garbage data is removed to prevent space waste
   - **If kept:** The overwritten file remains on disk for verification or other purposes

4. **Platform Compatibility:** 
   - **Windows:** Uses `DeviceIoControl` with `FSCTL_FILE_LEVEL_TRIM` for TRIM operations
   - **Linux:** Uses `/sys/block/*/queue/rotational` for detection and `fallocate()` with `FALLOC_FL_PUNCH_HOLE` for TRIM
   
5. **Use Responsibly:** Always verify the target file path before confirming the operation.

## How It Works: Security Features

### Issues Addressed

**1. Garbage Data Space Problem:**
- After overwriting, garbage values previously remained on disk, wasting space
- Solution: Optional file deletion frees space and removes forensic traces

**2. SSD Wear-Leveling Problem:**
- SSDs don't overwrite data in-place; they write to new locations
- Solution: TRIM commands inform the SSD controller to deallocate and eventually erase old blocks

### Storage-Specific Handling

**For HDDs:**
- Overwrites data with multiple passes
- If deleted, frees space immediately
- Garbage data is removed from disk

**For SSDs:**
- Overwrites data (physical location may change due to wear-leveling)
- Issues TRIM commands to mark logical blocks as unused
- SSD controller performs garbage collection asynchronously
- Reduces forensic traces and prevents space waste

### Secure Deletion Workflow

1. **Shred Phase:** Overwrite file with multiple passes (0x00, 0xFF, random data)
2. **User Prompt:** Choose whether to delete the file
3. **Detection Phase:** Determine if storage is SSD or HDD (if deleting)
4. **TRIM Phase:** Issue TRIM/DISCARD commands (SSD only)
5. **Deletion Phase:** Remove file from file system
6. **Verification:** Confirm deletion and space freeing

## Use Cases

- Secure deletion of sensitive documents
- Preparing storage devices for disposal
- Removing confidential data before system transfers
- Compliance with data destruction policies
- Testing parallel I/O performance

## Contributing

Contributions are welcome! Areas for improvement:

- Additional overwrite patterns (DoD 5220.22-M, Gutmann method)
- Hardware-based secure erase integration (ATA Secure Erase)
- Progress bars and improved UI
- Batch file processing
- Support for NVMe-specific sanitize commands
- macOS support

## References

- [OpenMP Specification](https://www.openmp.org/)
- [Windows DeviceIoControl API](https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-deviceiocontrol)
- [Linux fallocate System Call](https://man7.org/linux/man-pages/man2/fallocate.2.html)
- [TRIM Command Overview](https://en.wikipedia.org/wiki/Trim_(computing))
- [SSD Wear-Leveling](https://en.wikipedia.org/wiki/Wear_leveling)

## License

Open source software. Use at your own risk. The authors are not responsible for data loss or misuse.
