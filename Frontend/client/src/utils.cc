// utils.cc
#include "utils.h"
#include <unistd.h>    // For read() and write()
#include <cerrno>      // For errno
#include <cstring>     // For strerror
#include <string>
#include <random>
#include <cctype>
#include <vector>
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

std::string base64_encode(const std::string &data) {
    std::string encoded;
    int val = 0;
    int valb = -6;

    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded.size() % 4) {
        encoded.push_back('=');
    }
    return encoded;
}

std::string base64_decode(const std::string &encoded) {
    std::vector<int> decoding_table(256, -1);
    for (int i = 0; i < 64; i++) {
        decoding_table[base64_chars[i]] = i;
    }

    std::string decoded;
    int val = 0;
    int valb = -8;

    for (unsigned char c : encoded) {
        if (decoding_table[c] == -1) break;
        val = (val << 6) + decoding_table[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

std::string get_current_time(){
    time_t now = time(0);         
    std::string current_time = ctime(&now);
    return current_time;
}

// Generate a random ID
std::string generate_random_id() {
    // Random number generator
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 61); // Values from 0-61 (for 0-9, a-z, A-Z)

    const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string random_id;
    for (int i = 0; i < 32; ++i) { // Generate 32-character random_id
        random_id += chars[dis(gen)];
    }
    return random_id;
}

// Decode a URL-encoded string (replace %20 with space....etc.)
std::string url_decode(const std::string& encoded) {
    std::string decoded;
    size_t length = encoded.length();

    for (size_t i = 0; i < length; ++i) {
        if (encoded[i] == '%') {
            std::string hex_value = encoded.substr(i + 1, 2);
            char decoded_char = static_cast<char>(std::strtol(hex_value.c_str(), nullptr, 16));
            decoded += decoded_char;
            i += 2;
        } else {
            decoded += encoded[i];
        }
    }

    while (decoded.find("+") != std::string::npos) {
        size_t pos = decoded.find("+");
        decoded.replace(pos, 1, " ");
    }

    return decoded;
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length(); // Move past the replaced substring
    }
}


