// File.h
#ifndef FILE_H
#define FILE_H

#include <string>
// File struct to store the name and data of a file uploaded by the user
struct File {
    std::string name;
    std::string data;
};

#endif // FILE_H