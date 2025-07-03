// utils.cc
#include "utils.h"      // Header for utility function declarations
#include <unistd.h>     // For read() and write()
#include <cerrno>       // For errno
#include <cstring>      // For strerror
#include <string>       // For std::string

// Function to write a string to the given file descriptor
int wrapped_write(int comm_fd, const std::string& data) {
    size_t total_bytes_written = 0;  // Total bytes sent so far
    ssize_t bytes_written;
    size_t length = data.size();

    while (total_bytes_written < length) {
        // Attempt to write the remaining data
        bytes_written = write(comm_fd, data.c_str() + total_bytes_written, length - total_bytes_written);
        if (bytes_written <= 0) {
            if (bytes_written < 0 && (errno == EINTR || errno == EAGAIN)) {
                continue; // Retry if interrupted
            }
            return -1;  // Error occurred
        }
        total_bytes_written += bytes_written;
    }
    return total_bytes_written;  // Return total bytes written on success
}

// Function to read from the given file descriptor into a string buffer
int wrapped_read(int fd, std::string* buf) {
    char buffer[1024];
    ssize_t read_count;
    size_t total_bytes_read = 0;

    while (true) {
        read_count = read(fd, buffer, sizeof(buffer));
        if (read_count == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue; // Retry if interrupted
            }
            return -1; // Error occurred
        }
        if (read_count == 0) {
            break;
        }

        // Append data to buf and update total bytes read
        buf->append(buffer, read_count);
        total_bytes_read += read_count;

        // Exit loop after a successful read to allow processing the request
        break;
    }

    return total_bytes_read;
}


