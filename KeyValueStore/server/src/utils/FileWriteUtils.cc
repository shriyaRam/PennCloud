#include <fcntl.h>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include <string>

#include "FileWriteUtils.h"
#include <fstream>
#include "Logger.h"

#include <stdexcept>

void logOperation(const std::string &filename, const std::string &operation, const std::string &row, const std::string &column, const std::string &value1, const std::string &value2, const int serial)
{
    std::ofstream file;
    file.open(filename, std::ios::app);

    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open log file: " << filename << std::endl;
        return;
    }

    // Write log entry
    file << "\n"; // Empty line before operation
    if (operation == "PUT")
    {
        file << serial << ",PUT," << row << "," << column << "," << value1 << "\n";
    }
    else if (operation == "DELETE")
    {
        file << serial << ",DELETE," << row << "," << column << "\n";
    }
    else if (operation == "CPUT")
    {
        file << serial << ",CPUT," << row << "," << column << "," << value1 << "," << value2 << "\n";
    }
    else
    {
        std::cerr << "Error: Invalid operation specified: " << operation << std::endl;
    }
    file << "END\n"; // Add marker to denote the end of the operation
    // file << "\n";    // Empty line after operation
    file.close();
}

void writeLogRecord(const std::string &filename, const std::string &operation, const std::string &row,
                    const std::string &column, const std::string &value1, const std::string &value2)
{
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    // Determine the number of columns
    uint32_t numColumns = 3; // Operation, Row, Column
    if (operation == "PUT")
        numColumns = 4; // Includes Value1
    else if (operation == "CPUT")
        numColumns = 5; // Includes Value1 and Value2

    // Create the record string
    std::string record = operation + "," + row + "," + column;
    if (operation == "PUT" || operation == "CPUT")
    {
        record += "," + value1;
    }
    if (operation == "CPUT")
    {
        record += "," + value2;
    }

    // Encode the record length
    uint32_t recordLength = record.size() + sizeof(numColumns); // Include the column count
    if (write(fd, &recordLength, sizeof(recordLength)) != sizeof(recordLength))
    {
        throw std::runtime_error("Error: Failed to write record length");
    }

    // Encode the number of columns
    if (write(fd, &numColumns, sizeof(numColumns)) != sizeof(numColumns))
    {
        throw std::runtime_error("Error: Failed to write column count");
    }

    // Write the record data
    if (write(fd, record.c_str(), record.size()) != static_cast<ssize_t>(record.size()))
    {
        throw std::runtime_error("Error: Failed to write record data");
    }

    // Close the file
    close(fd);
}

void logTransaction(const std::string &filename, const int serial, const std::string &operation)
{
    std::ofstream file;
    file.open(filename, std::ios::app);

    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open log file: " << filename << std::endl;
        return;
    }

    file << operation << "TRANSACTION " << std::to_string(serial) << std::endl;
}

void updateSnapshot(const std::string &filename, BigTable &table)
{
    std::ofstream file;
    file.open(filename, std::ios::out | std::ios::trunc);

    if (!file.is_open())
    {
        std::cerr << "Error: Failed to open snapshot file: " << filename << std::endl;
        return;
    }

    // sort keys in the table
    vector<string> keys;
    auto tablePtr = table.getTablePointer();
    for (auto it = tablePtr->begin(); it != tablePtr->end(); it++)
    {
        keys.push_back(it->first);
    }

    std::sort(keys.begin(), keys.end());

    // now we will do the following
    // calculate length of row key and encode it in 32 length b
}

void deleteLastLineEfficiently(const std::string &filename)
{
    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Get the position at the end of the file
    std::streampos endPos = file.tellg();
    std::streamoff truncatePos = endPos; // Use streamoff for arithmetic

    char ch;

    // Move backwards to find the beginning of the last line
    while (truncatePos > 0)
    {
        file.seekg(--truncatePos, std::ios::beg); // Use streamoff for arithmetic
        file.get(ch);
        if (ch == '\n' && truncatePos != endPos - 1)
        {
            truncatePos++; // Move to the character after '\n'
            break;
        }
    }

    // Truncate the file at the new position
    file.close(); // Close the file before truncating

    std::ofstream truncateFile(filename, std::ios::in | std::ios::out | std::ios::trunc);
    truncateFile.close();
    logWithTimestamp("") << "Truncated.\n";
}

void overwriteFile(const std::filesystem::path &sourcePath, const std::filesystem::path &targetPath)
{
    // clearFile(targetPath);
    // Construct the temporary file name
    std::filesystem::path tempFilePath = targetPath.parent_path() / (targetPath.stem().string() + "_temp" + targetPath.extension().string());
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Temporary file path: " << tempFilePath << std::endl;

    // Open the source file for reading
    int ensureFileDescriptor = open(sourcePath.c_str(), O_CREAT | O_WRONLY, 0666);
    if (ensureFileDescriptor == -1)
    {
        std::cerr << "Error: Failed to create the file: " << sourcePath << std::endl;
        return;
    }
    close(ensureFileDescriptor);
    int sourceFileDescriptor = open(sourcePath.c_str(), O_RDONLY);
    if (sourceFileDescriptor == -1)
    {
        std::cerr << "Error: Could not open source file: " << sourcePath << std::endl;
        return;
    }

    // Open the temporary file for writing (create or truncate)
    int tempFileDescriptor = open(tempFilePath.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (tempFileDescriptor == -1)
    {
        std::cerr << "Error: Could not create or open the temp file: " << tempFilePath << std::endl;
        close(sourceFileDescriptor);
        return;
    }

    // Read from source and write to temp
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(sourceFileDescriptor, buffer, sizeof(buffer))) > 0)
    {
        if (write(tempFileDescriptor, buffer, bytesRead) == -1)
        {
            std::cerr << "Error: Failed to write to the temp file." << std::endl;
            close(sourceFileDescriptor);
            close(tempFileDescriptor);
            return;
        }
    }

    if (bytesRead == -1)
    {
        std::cerr << "Error: Failed to read from the source file." << std::endl;
    }

    // Close file descriptors
    close(sourceFileDescriptor);
    close(tempFileDescriptor);

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Temporary file written successfully: " << tempFilePath << std::endl;
    if (std::filesystem::exists(targetPath))
    {
        if (!std::filesystem::remove(targetPath))
        {
            std::cerr << "Error: Failed to delete the existing target file: " << targetPath << std::endl;
            return;
        }
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Existing target file deleted: " << targetPath << std::endl;
    }

    // Rename the temporary file to the target file
    try
    {
        std::filesystem::rename(tempFilePath, targetPath);
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Temporary file renamed to target file: " << targetPath << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: Failed to rename temp file to target file: " << e.what() << std::endl;
        return;
    }
    // Placeholder for renaming logic
    // Future: Add code to rename temp file to the target file
}

void clearFile(const std::filesystem::path &filePath)
{
    // Open the file in truncate mode to clear its content
    std::ofstream file(filePath, std::ios::trunc);
    if (!file)
    {
        throw std::runtime_error("Error: Unable to clear file: " + filePath.string());
    }

    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Successfully cleared the content of " << filePath << std::endl;
}

void updateLineCount(const std::string &metaFile, size_t lineCount)
{
    // std::string metaFile = logFile;
    std::ofstream out(metaFile, std::ios::trunc);
    if (!out.is_open())
    {
        throw std::runtime_error("Failed to open metadata file for writing");
    }
    out << "line_count=" << lineCount << std::endl;
    out.close();
}

size_t getCurrentLineCount(const std::string &metaFile)
{
    // std::string metaFile = logFile + ".meta";
    std::ifstream in(metaFile);
    if (!in.is_open())
    {
        return 0; // Default to 0 if metadata file doesn't exist
    }

    std::string line;
    size_t lineCount = 0;
    while (std::getline(in, line))
    {
        if (line.find("line_count=") == 0)
        {
            lineCount = std::stoull(line.substr(11)); // Extract the number
            break;
        }
    }
    in.close();
    return lineCount;
}
