// Parallel Digital Shredder - Utility Functions
// File operations, validation, and random data generation

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <random>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

using namespace std;

long get_file_size(FILE* file) {
    if (!file) {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        return -1;
    }

    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    return size;
}

bool validate_file(const char* path) {
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        cerr << "Error: File does not exist: " << path << "\n";
        return false;
    }

    if (!S_ISREG(file_stat.st_mode)) {
        cerr << "Error: Not a regular file: " << path << "\n";
        return false;
    }

    FILE* test_file = fopen(path, "rb+");
    if (!test_file) {
        cerr << "Error: File is not writable: " << path << "\n";
        return false;
    }
    fclose(test_file);

    if (file_stat.st_size == 0) {
        cerr << "Error: File is empty: " << path << "\n";
        return false;
    }

    return true;
}

void fill_random_bytes(unsigned char* buffer, long size) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(0, 255);

    for (long i = 0; i < size; i++) {
        buffer[i] = static_cast<unsigned char>(dis(gen));
    }
}

void print_warning() {
    cout << "\n";
    cout << "╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║              ⚠️  DESTRUCTIVE OPERATION WARNING  ⚠️         ║\n";
    cout << "╠═══════════════════════════════════════════════════════════╣\n";
    cout << "║  This program will PERMANENTLY OVERWRITE the target file  ║\n";
    cout << "║  Data recovery will be IMPOSSIBLE after this operation    ║\n";
    cout << "║  Make sure you have selected the correct file!            ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n";
    cout << "\n";
}

bool get_user_confirmation() {
    string response;
    getline(cin, response);

    // Trim whitespace
    response.erase(response.begin(), find_if(response.begin(), response.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
    response.erase(find_if(response.rbegin(), response.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), response.end());

    // Convert to lowercase for comparison
    transform(response.begin(), response.end(), response.begin(), [](unsigned char c) {
        return tolower(c);
    });

    return (response == "y" || response == "yes");
}
