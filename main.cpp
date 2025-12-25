// Parallel Digital Shredder - PDC Project
// Demonstrates: Shared-memory parallelism, data parallelism, OpenMP work-sharing, thread synchronization

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <omp.h>
#include "shredder.h"

using namespace std;

// Forward declarations from utils.cpp
long get_file_size(FILE* file);
bool validate_file(const char* path);
void print_warning();
bool get_user_confirmation();

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        cerr << "Usage: " << argv[0] << " <file_path> <passes> [threads]\n";
        cerr << "  file_path: target file to shred\n";
        cerr << "  passes:    number of overwrite passes (>=1)\n";
        cerr << "  threads:   optional thread count (default: system maximum)\n";
        return 1;
    }

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

    if (!validate_file(file_path)) {
        cerr << "Error: File validation failed\n";
        return 1;
    }

    print_warning();
    cout << "File: " << file_path << "\n";
    cout << "This operation is IRREVERSIBLE. Continue? (y/yes or n/no): ";
    
    if (!get_user_confirmation()) {
        cout << "Operation cancelled by user.\n";
        return 0;
    }

    // Shared FILE* among threads - each thread uses fseek() before writing
    FILE* file = fopen(file_path, "rb+");
    if (!file) {
        cerr << "Error: Cannot open file for writing\n";
        return 1;
    }

    long file_size = get_file_size(file);
    if (file_size <= 0) {
        cerr << "Error: Invalid file size\n";
        fclose(file);
        return 1;
    }

    // Data parallelism: divide file into equal chunks
    long chunk_size = file_size / num_threads;
    long remainder = file_size % num_threads;

    cout << "\n=== Parallel Shredding Configuration ===\n";
    cout << "File size:        " << file_size << " bytes\n";
    cout << "Thread count:     " << num_threads << "\n";
    cout << "Overwrite passes: " << passes << "\n";
    cout << "Chunk size:       " << chunk_size << " bytes\n";
    cout << "Remainder:        " << remainder << " bytes (handled by last thread)\n";
    cout << "========================================\n\n";

    omp_set_num_threads(num_threads);
    auto start_time = chrono::high_resolution_clock::now();

    // OpenMP parallel region: each thread processes its chunk
    #pragma omp parallel for schedule(static)
    for (int tid = 0; tid < num_threads; tid++) {
        long start_offset = tid * chunk_size;
        long current_chunk_size = chunk_size;
        
        if (tid == num_threads - 1) {
            current_chunk_size += remainder;
        }

        // Synchronization: omp critical prevents garbled console output
        #pragma omp critical(console_output)
        {
            cout << "Thread " << tid << ": Processing bytes " 
                      << start_offset << " to " 
                      << (start_offset + current_chunk_size - 1) << "\n";
        }

        shred_chunk(file, start_offset, current_chunk_size, passes);

        #pragma omp critical(console_output)
        {
            cout << "Thread " << tid << ": Completed\n";
        }
    }

    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(
        end_time - start_time
    );

    fflush(file);
    fclose(file);

    cout << "\n.... Shredding Complete ....\n";
    cout << "Total execution time: " << duration.count() << " ms\n";
    cout << "Throughput: " 
              << (file_size * passes / (duration.count() / 1000.0) / (1024 * 1024))
              << " MB/s\n";
    cout << ".......................................\n";

    cout << "\nFile securely overwritten with " << passes << " passes.\n";
    cout << "Note: For maximum security, the file should now be deleted.\n";

    return 0;
}
