#ifndef CLEAR_FILES_H
#define CLEAR_FILES_H

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Function to clear file contents in a specific folder with a given file extension
void clearFilesInFolder(const fs::path &basePath, const std::string &subFolder, const std::string &fileExtension)
{
    fs::path targetPath = basePath / subFolder;

    if (fs::exists(targetPath) && fs::is_directory(targetPath))
    {
        for (const auto &entry : fs::directory_iterator(targetPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == fileExtension)
            {
                std::ofstream ofs(entry.path(), std::ios::trunc); // Open in truncate mode to clear the content
                if (ofs)
                {
                    std::cout << "Cleared content of: " << entry.path() << std::endl;
                }
                else
                {
                    std::cerr << "Failed to clear content of: " << entry.path() << std::endl;
                }
            }
        }
    }
    else
    {
        std::cerr << "Directory does not exist or is not accessible: " << targetPath << std::endl;
    }
}

// Function to process multiple folders and clear specified files
void processFolders(const std::vector<std::string> &folderPaths)
{
    for (const auto &folderPath : folderPaths)
    {
        fs::path basePath(folderPath);

        if (fs::exists(basePath) && fs::is_directory(basePath))
        {
            std::cout << "Processing folder: " << basePath << std::endl;

            // Clear .txt files in the logs subfolder
            clearFilesInFolder(basePath, "logs", ".txt");

            // Clear .bin files in the snapshots subfolder
            clearFilesInFolder(basePath, "snapshots", ".bin");
        }
        else
        {
            std::cerr << "Invalid folder path: " << folderPath << std::endl;
        }
    }
}

#endif // CLEAR_FILES_H