// Parallel Digital Shredder - Core Shredding Logic
// Thread-private buffers, independent chunk processing with fseek()

#ifndef SHREDDER_H
#define SHREDDER_H

#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <omp.h>

using namespace std;

void fill_random_bytes(unsigned char* buffer, long size);

void shred_chunk(FILE* file, long start_offset, long chunk_size, int passes) {
    const long BUFFER_SIZE = 1024 * 1024; // 1 MB
    unsigned char* buffer = new unsigned char[BUFFER_SIZE];
    
    if (!buffer) {
        return;
    }

    for (int pass = 0; pass < passes; pass++) {
        unsigned char pattern;
        bool use_random = false;
        
        if (pass % 3 == 0) {
            pattern = 0x00;
        } else if (pass % 3 == 1) {
            pattern = 0xFF;
        } else {
            use_random = true;
        }

        long bytes_remaining = chunk_size;
        long current_offset = start_offset;

        while (bytes_remaining > 0) {
            long bytes_to_write = (bytes_remaining < BUFFER_SIZE) ? 
                                   bytes_remaining : BUFFER_SIZE;

            if (use_random) {
                fill_random_bytes(buffer, bytes_to_write);
            } else {
                memset(buffer, pattern, bytes_to_write);
            }

            // Seek to correct position - ensures thread writes only its chunk
            if (fseek(file, current_offset, SEEK_SET) != 0) {
                delete[] buffer;
                return;
            }

            size_t written = fwrite(buffer, 1, bytes_to_write, file);
            
            if (written != static_cast<size_t>(bytes_to_write)) {
                delete[] buffer;
                return;
            }

            current_offset += bytes_to_write;
            bytes_remaining -= bytes_to_write;
        }

        fflush(file);
    }

    delete[] buffer;
}

#endif // SHREDDER_H
