// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <string>

// Declaration of wrapped_write: Writes a string to the specified file descriptor
int wrapped_write(int comm_fd, const std::string& data);

// Declaration of wrapped_read: Reads from the specified file descriptor into a string buffer
int wrapped_read(int fd, std::string* buf);

const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

// Encoding to base64 (Not in use)
std::string base64_encode(const std::string &data);

// Decoding to base64 (Not in use)
std::string base64_decode(const std::string &data);

// Get the current time in the format: YYYY-MM-DD HH:MM:SS
std::string get_current_time();

// Generate a random ID
std::string generate_random_id();

// Decode a URL-encoded string (replace %20 with space....etc.)
std::string url_decode(const std::string& encoded);

#endif // UTILS_H