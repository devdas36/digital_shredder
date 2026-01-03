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

#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#include <fileapi.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#endif

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

void print_banner() {
    cout << "\n";
    cout << "  _____ _              _     _           \n";
    cout << " / ____| |            | |   | |          \n";
    cout << "| (___ | |__  _ __ ___| | __| | ___ _ __ \n";
    cout << " \\___ \\| '_ \\| '__/ _ \\ |/ _` |/ _ \\ '__|\n";
    cout << " ____) | | | | | |  __/ | (_| |  __/ |   \n";
    cout << "|_____/|_| |_|_|  \\___|_|\\__,_|\\___|_|\n";
    cout << "\nParallel Digital Shredder - Secure File Deletion\n";
}

void print_warning() {
    cout << "\nWARNING: This will PERMANENTLY overwrite the file\n";
    cout << "         Data recovery will be IMPOSSIBLE\n";
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

bool get_deletion_confirmation() {
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

#ifdef _WIN32
// Detect if the file is on an SSD by querying Windows storage properties
bool is_ssd(const char* path) {
    // Get the volume path from the file path
    char volume_path[MAX_PATH];
    if (!GetVolumePathNameA(path, volume_path, MAX_PATH)) {
        cerr << "Warning: Could not determine volume path\n";
        return false;
    }

    // Format volume path for DeviceIoControl (e.g., "\\\\.\\C:")
    char device_path[MAX_PATH];
    snprintf(device_path, MAX_PATH, "\\\\.\\%c:", volume_path[0]);

    HANDLE hDevice = CreateFileA(
        device_path,
        0,  // No access needed for query
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        cerr << "Warning: Could not open device for query\n";
        return false;
    }

    STORAGE_PROPERTY_QUERY query;
    memset(&query, 0, sizeof(query));
    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    query.QueryType = PropertyStandardQuery;

    DEVICE_SEEK_PENALTY_DESCRIPTOR result;
    memset(&result, 0, sizeof(result));
    DWORD bytes_returned = 0;

    BOOL success = DeviceIoControl(
        hDevice,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &query,
        sizeof(query),
        &result,
        sizeof(result),
        &bytes_returned,
        NULL
    );

    CloseHandle(hDevice);

    if (success && bytes_returned >= sizeof(result)) {
        // IncursSeekPenalty = FALSE means it's an SSD
        return !result.IncursSeekPenalty;
    }

    cerr << "Warning: Could not determine if device is SSD\n";
    return false;
}

// Perform TRIM operation on the file to free space on SSDs
bool trim_file(const char* path, long file_size) {
    // TRIM is most effective for larger files
    // For very small files, the overhead may not be worth it
    if (file_size < 4096) {
        return true; // Skip TRIM for files smaller than 4KB
    }

    HANDLE hFile = CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,  // Normal file attributes
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        // File might already be deleted or inaccessible
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
            return true; // File already gone, consider it success
        }
        return false;
    }

    // Get actual file size to ensure it matches
    LARGE_INTEGER file_size_check;
    if (!GetFileSizeEx(hFile, &file_size_check)) {
        CloseHandle(hFile);
        return false;
    }

    // Structure for FILE_LEVEL_TRIM
    struct {
        DWORD Key;
        DWORD NumRanges;
        struct {
            ULONGLONG Offset;
            ULONGLONG Length;
        } Ranges[1];
    } trim_data;

    memset(&trim_data, 0, sizeof(trim_data));
    trim_data.Key = 0;
    trim_data.NumRanges = 1;
    trim_data.Ranges[0].Offset = 0;
    trim_data.Ranges[0].Length = file_size_check.QuadPart;

    DWORD bytes_returned = 0;
    BOOL success = DeviceIoControl(
        hFile,
        FSCTL_FILE_LEVEL_TRIM,
        &trim_data,
        sizeof(trim_data),
        NULL,
        0,
        &bytes_returned,
        NULL
    );

    DWORD last_error = GetLastError();
    CloseHandle(hFile);

    if (!success) {
        // Error 312 = ERROR_NO_RANGES_PROCESSED might occur on some file systems
        // This is not critical - the file system may not support file-level TRIM
        if (last_error == 312 || last_error == ERROR_NOT_SUPPORTED || 
            last_error == ERROR_INVALID_FUNCTION) {
            // These errors are acceptable - file system doesn't support TRIM at file level
            // The volume-level TRIM will handle it during normal operations
            return true;
        }
        return false;
    }

    return true;
}
#else
// Linux implementations for SSD detection and TRIM operations
bool is_ssd(const char* path) {
    // Get the device path from the file path
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        cerr << "Warning: Could not stat file for SSD detection\n";
        return false;
    }

    // Get the device name
    char device_path[256];
    FILE* mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        cerr << "Warning: Could not read /proc/mounts\n";
        return false;
    }

    char line[1024];
    char mount_device[256] = "";
    char mount_point[256] = "";
    char longest_match[256] = "";
    int longest_length = 0;

    // Find the mount point for this file
    while (fgets(line, sizeof(line), mounts)) {
        if (sscanf(line, "%255s %255s", mount_device, mount_point) == 2) {
            int len = strlen(mount_point);
            if (strncmp(path, mount_point, len) == 0 && len > longest_length) {
                longest_length = len;
                strncpy(longest_match, mount_device, sizeof(longest_match) - 1);
                longest_match[sizeof(longest_match) - 1] = '\0';
            }
        }
    }
    fclose(mounts);

    if (longest_length == 0) {
        cerr << "Warning: Could not determine device for file\n";
        return false;
    }

    // Extract the base device name (e.g., sda from /dev/sda1)
    char base_device[256];
    const char* dev_name = strrchr(longest_match, '/');
    if (!dev_name) {
        dev_name = longest_match;
    } else {
        dev_name++;
    }

    // Remove partition numbers
    int i = 0;
    while (dev_name[i] && !isdigit(dev_name[i]) && i < 255) {
        base_device[i] = dev_name[i];
        i++;
    }
    base_device[i] = '\0';

    // Check if rotational (0 = SSD, 1 = HDD)
    snprintf(device_path, sizeof(device_path), "/sys/block/%s/queue/rotational", base_device);
    FILE* rotational = fopen(device_path, "r");
    if (!rotational) {
        cerr << "Warning: Could not determine if device is SSD\n";
        return false;
    }

    char value[8];
    if (fgets(value, sizeof(value), rotational)) {
        fclose(rotational);
        return (value[0] == '0');
    }

    fclose(rotational);
    return false;
}

bool trim_file(const char* path, long file_size) {
    // TRIM is most effective for larger files
    if (file_size < 4096) {
        return true; // Skip TRIM for files smaller than 4KB
    }

    int fd = open(path, O_RDWR);
    if (fd < 0) {
        // File might already be deleted or inaccessible
        if (errno == ENOENT) {
            return true; // File already gone, consider it success
        }
        return false;
    }

    // Use BLKDISCARD ioctl for TRIM on Linux
    // First check if file is on a regular file system
    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return false;
    }

    if (!S_ISREG(st.st_mode)) {
        close(fd);
        return false;
    }

    // For regular files, we use FALLOC_FL_PUNCH_HOLE to deallocate space
    #ifdef FALLOC_FL_PUNCH_HOLE
    int result = fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 0, file_size);
    close(fd);
    
    if (result == 0) {
        return true;
    }
    
    // If FALLOC_FL_PUNCH_HOLE is not supported, it's not critical
    if (errno == EOPNOTSUPP || errno == ENOSYS) {
        return true; // File system doesn't support punch hole, but that's okay
    }
    #else
    close(fd);
    #endif
    
    return true; // Return true even if not supported
}
#endif

// Delete the file and free disk space
bool secure_delete_file(const char* path, bool is_ssd_device, long file_size) {
    if (is_ssd_device) {
        trim_file(path, file_size);
    }

    // Delete the file
    if (remove(path) != 0) {
        cerr << "Error: Failed to delete file: " << path << "\n";
        return false;
    }

    return true;
}
