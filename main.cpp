// Demonstrates: Shared-memory parallelism, data parallelism, OpenMP work-sharing, thread synchronization

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <thread>
#include <omp.h>
#include "shredder.h"

using namespace std;

// Forward declarations from utils.cpp
long get_file_size(FILE* file);
bool validate_file(const char* path);
void print_warning();
void print_banner();
bool get_user_confirmation();
bool get_deletion_confirmation();
bool is_ssd(const char* path);
bool secure_delete_file(const char* path, bool is_ssd_device, long file_size);

// External progress tracking variables
extern volatile long total_bytes_processed;
extern long total_bytes_to_process;
extern int current_pass;
extern int total_passes;

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        cerr << "\nUsage: " << argv[0] << " <file_path> <passes> [threads]\n\n";
        cerr << "Arguments:\n";
        cerr << "  file_path    Target file to shred\n";
        cerr << "  passes       Number of overwrite passes (min: 1)\n";
        cerr << "  threads      Number of threads (optional, default: auto)\n\n";
        cerr << "Examples:\n";
        cerr << "  " << argv[0] << " secret.txt 3\n";
        cerr << "  " << argv[0] << " document.pdf 7 4\n\n";
        return 1;
    }

    print_banner();

    const char* file_path = argv[1];
    int passes = atoi(argv[2]);
    int num_threads = (argc == 4) ? atoi(argv[3]) : omp_get_max_threads();

    if (passes < 1) {
        cerr << "Error: Number of passes must be at least 1\n";
        return 1;
    }

    if (num_threads < 1) {
        cerr << "Error: Number of threads must be at least 1\n";
        return 1;
    }

    cout << "\nValidating " << file_path << " ...\n";

    if (!validate_file(file_path)) {
        cerr << "Error: File validation failed\n";
        return 1;
    }
    
    cout << "  + File OK\n";

    cout << "  + File OK\n";

    // Detect if the storage device is an SSD
    bool is_ssd_device = is_ssd(file_path);
    if (is_ssd_device) {
        cout << "  + Storage: SSD detected (TRIM will be used)\n";
    } else {
        cout << "  + Storage: HDD/Standard\n";
    }

    print_warning();
    cout << "\nContinue? (y/n): ";
    
    if (!get_user_confirmation()) {
        cout << "\nOperation cancelled\n";
        return 0;
    }

    // Shared FILE* among threads - each thread uses fseek() before writing
    FILE* file = fopen(file_path, "rb+");
    if (!file) {
        cerr << "\nError: Cannot open file for writing\n";
        return 1;
    }

    long file_size = get_file_size(file);
    if (file_size <= 0) {
        cerr << "\nError: Invalid file size\n";
        fclose(file);
        return 1;
    }

    // Data parallelism: divide file into equal chunks
    long chunk_size = file_size / num_threads;
    long remainder = file_size % num_threads;
    
    char size_buffer[50];
    format_bytes(file_size, size_buffer, sizeof(size_buffer));

    cout << "\nConfiguration:\n";
    cout << "  Size: " << size_buffer << " | Passes: " << passes << " | Threads: " << num_threads << "\n";
    cout << "\nShredding...\n";

    // Initialize progress tracking
    total_bytes_to_process = file_size;
    total_passes = passes;

    omp_set_num_threads(num_threads);
    auto start_time = chrono::high_resolution_clock::now();

    // Progress monitoring in separate section
    for (int pass = 1; pass <= passes; pass++) {
        current_pass = pass;
        total_bytes_processed = 0;
        
        const char* pattern_name;
        if ((pass - 1) % 3 == 0) pattern_name = "0x00";
        else if ((pass - 1) % 3 == 1) pattern_name = "0xFF";
        else pattern_name = "rand";

        // OpenMP parallel region: each thread processes its chunk
        #pragma omp parallel for schedule(static)
        for (int tid = 0; tid < num_threads; tid++) {
            long start_offset = tid * chunk_size;
            long current_chunk_size = chunk_size;
            
            if (tid == num_threads - 1) {
                current_chunk_size += remainder;
            }

            // Process one pass
            const long BUFFER_SIZE = 1024 * 1024; // 1 MB
            unsigned char* buffer = new unsigned char[BUFFER_SIZE];
            
            if (!buffer) continue;

            unsigned char pattern;
            bool use_random = false;
            
            if ((pass - 1) % 3 == 0) {
                pattern = 0x00;
            } else if ((pass - 1) % 3 == 1) {
                pattern = 0xFF;
            } else {
                use_random = true;
            }

            long bytes_remaining = current_chunk_size;
            long current_offset = start_offset;

            while (bytes_remaining > 0) {
                long bytes_to_write = (bytes_remaining < BUFFER_SIZE) ? 
                                       bytes_remaining : BUFFER_SIZE;

                if (use_random) {
                    fill_random_bytes(buffer, bytes_to_write);
                } else {
                    memset(buffer, pattern, bytes_to_write);
                }

                if (fseek(file, current_offset, SEEK_SET) != 0) {
                    delete[] buffer;
                    break;
                }

                size_t written = fwrite(buffer, 1, bytes_to_write, file);
                
                if (written != static_cast<size_t>(bytes_to_write)) {
                    delete[] buffer;
                    break;
                }

                current_offset += bytes_to_write;
                bytes_remaining -= bytes_to_write;
                
                // Update progress
                #pragma omp atomic
                total_bytes_processed += bytes_to_write;
            }

            fflush(file);
            delete[] buffer;
        }
        
        // Show completion for this pass
        cout << "  Pass " << pass << "/" << passes << " (" << pattern_name << ") ";
        display_progress_bar(100, pass, passes);
        cout << " done\n";
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(
        end_time - start_time
    );

    fflush(file);
    fclose(file);

    cout << "\nCompleted in " << duration.count() << " ms";
    cout << " (" << fixed << setprecision(2)
              << (file_size * passes / (duration.count() / 1000.0) / (1024 * 1024))
              << " MB/s)\n";

    cout << "\nDelete file? (y/n): ";
    
    if (!get_deletion_confirmation()) {
        cout << "\nFile kept (overwritten data remains on disk)\n\n";
        return 0;
    }
    
    // Perform secure deletion and space freeing
    cout << "\nDeleting...\n";
    
    bool deletion_success = secure_delete_file(file_path, is_ssd_device, file_size);
    
    if (deletion_success) {
        cout << "  + File deleted successfully\n";
        if (is_ssd_device) {
            cout << "  + TRIM issued (SSD will free blocks)\n";
        }
    } else {
        cout << "  ! Deletion failed (manual removal may be needed)\n";
    }
    
    cout << "\n";

    return 0;
}
