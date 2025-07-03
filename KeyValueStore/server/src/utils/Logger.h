#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

// Function to get the current timestamp as a string
inline std::string getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// Inline function to log messages with a timestamp
inline std::ostream &logWithTimestamp(const std::string &serverAddress)
{
    return std::cout << "[" << getCurrentTimestamp() << "] "
                     << "[" << serverAddress << "] ";
}

#endif // LOGGER_H
