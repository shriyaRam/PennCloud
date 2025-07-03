#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include "BigTable.h"
#include "checkpoint.h"
#include <memory>
#include "Logger.h"
#include <map>

using namespace std;
// Function to decode a 32-byte length
uint32_t decodeLength(std::ifstream &file)
{
    uint8_t buffer[4];
    file.read(reinterpret_cast<char *>(buffer), 4);
    if (!file)
    {
        throw std::runtime_error("Failed to read length from file");
    }

    uint32_t length = 0;
    length |= (buffer[0] << 24);
    length |= (buffer[1] << 16);
    length |= (buffer[2] << 8);
    length |= buffer[3];
    return length;
}

// Function to read a fixed number of bytes from the file
std::string readBytes(std::ifstream &file, size_t numBytes)
{
    char buffer[numBytes];
    file.read(buffer, numBytes);
    if (!file)
    {
        throw std::runtime_error("Failed to read bytes from file");
    }
    return std::string(buffer, numBytes);
}

// Function to load data into BigTable
void loadTableToBigTable(const std::string &filename, std::shared_ptr<BigTable> bigTable)
{
    logWithTimestamp("") << "[LOADTABLE] Loading table from file: " << filename << std::endl
                         << flush;

    // first we clear the bigtable particularly, we clear the map
    bigTable->getTablePointer()->clear();
    logWithTimestamp("") << "[LOADTABLE] BigTable cleared" << std::endl;
    logWithTimestamp("") << "[LOADTABLE] BigTable size: " << bigTable->getTablePointer()->size() << std::flush << endl;
    std::ifstream file(filename, std::ios::binary);
    logWithTimestamp("") << "[LOADTABLE] File maybe opened" << std::endl;
    if (!file.is_open())
    {
        cout << "File failed to open" << endl;
        throw std::runtime_error("Failed to open file for reading");
    }
    logWithTimestamp("") << "[LOADTABLE] File opened " << filename << std::endl
                         << std::flush;
    int checkpointVersion;
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0)
    {
        logWithTimestamp("") << "Empty file" << endl;
        file.close();
        return;
    }
    file.seekg(0, std::ios::beg);
    logWithTimestamp("") << "[LOADTABLE] File is not empty" << endl
                         << std::flush;

    file.read(reinterpret_cast<char *>(&checkpointVersion), sizeof(checkpointVersion));
    if (file.fail())
    {
        throw std::runtime_error("Failed to read checkpoint version");
    }
    // logWithTimestamp("") << "Loaded checkpoint version: " << checkpointVersion << std::endl;

    while (file.peek() != EOF)
    {
        // Decode row key length
        uint32_t rowKeyLength = decodeLength(file);

        // Read row key
        std::string rowKey = readBytes(file, rowKeyLength);

        // Decode number of columns
        uint32_t numCols = decodeLength(file);

        // Iterate over columns
        for (uint32_t i = 0; i < numCols; ++i)
        {
            uint32_t colKeyLength = decodeLength(file);
            std::string colKey = readBytes(file, colKeyLength);

            uint32_t colValueLength = decodeLength(file);
            std::string colValue = readBytes(file, colValueLength);

            // Add to BigTable
            bigTable->put(rowKey, colKey, colValue);
        }
    }

    file.close();
}

std::vector<uint8_t> encodeLength(uint32_t length)
{
    std::vector<uint8_t> encoded(4);    // 4 bytes for uint32_t
    encoded[0] = (length >> 24) & 0xFF; // Most significant byte
    encoded[1] = (length >> 16) & 0xFF;
    encoded[2] = (length >> 8) & 0xFF;
    encoded[3] = length & 0xFF; // Least significant byte
    return encoded;
}

// Encode a string into its byte representation
std::vector<uint8_t> encodeString(const std::string &str)
{
    return std::vector<uint8_t>(str.begin(), str.end());
}

// Write bytes to file
void writeBytes(std::ofstream &file, const std::vector<uint8_t> &bytes)
{
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

void serializeRow(const std::string &rowKey,
                  const std::unordered_map<std::string, std::string> &columns,
                  std::ofstream &file) // Pass the open file stream
{
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for writing");
    }

    // Step 1: Write row key length as uint32_t
    auto rowKeyLength = encodeLength(rowKey.size());
    writeBytes(file, rowKeyLength);

    // Step 2: Write row key
    file.write(rowKey.c_str(), rowKey.size());

    // Step 3: Write number of columns as uint32_t
    auto numCols = encodeLength(columns.size());
    writeBytes(file, numCols);

    // Step 4: Write each column and its value
    for (const auto &col : columns)
    {
        const auto &colKey = col.first;
        const auto &colValue = col.second;

        // Column key
        auto colKeyLength = encodeLength(colKey.size());
        writeBytes(file, colKeyLength);
        file.write(colKey.c_str(), colKey.size());

        // Column value
        auto colValueLength = encodeLength(colValue.size());
        writeBytes(file, colValueLength);
        file.write(colValue.c_str(), colValue.size());
    }
}

void serializeBigTable(std::shared_ptr<BigTable> bigTable, const std::string &outputFile, int32_t checkpointVersion)
{
    // Open the file in binary mode with truncation to overwrite it
    std::ofstream file(outputFile, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file for writing");
    }
    file.write(reinterpret_cast<const char *>(&checkpointVersion), sizeof(checkpointVersion));
    logWithTimestamp("") << "Checkpoint version " << checkpointVersion << " written to file" << std::endl;

    // Get the table pointer from BigTable
    auto tablePointer = bigTable->getTablePointer();
    // logWithTimestamp("") << "Before sorting 2" << endl;
    // logWithTimestamp("") << "Table pointer size: " << tablePointer << endl;
    for (auto it = tablePointer->begin(); it != tablePointer->end(); it++)
    {
        logWithTimestamp("") << it->first << endl;
    }
    // Sort the table based on the row key
    std::map<std::string, RowData> sortedTable(tablePointer->begin(), tablePointer->end());
    logWithTimestamp("") << "After sorting" << endl;
    // Iterate over all rows in the table
    for (const auto &row : sortedTable)
    {
        const std::string &rowKey = row.first;
        const RowData &rowData = row.second;
        printf("Serializing row %s\n", rowKey.c_str());

        // Serialize the row using the modified serializeRow function
        serializeRow(rowKey, *rowData.getRowPointer(), file);
    }

    // Close the file
    file.close();
    // logWithTimestamp("") << "File successfully overwritten: " << outputFile << std::endl;
}

int getCheckpointVersion(const std::string &filename)
{
    logWithTimestamp("") << "Getting checkpoint version from file: " << filename << std::endl;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 0; // Return 0 if the file cannot be opened
    }

    int checkpointVersion = 0;

    // Check if the file is empty
    file.seekg(0, std::ios::end);
    if (file.tellg() == 0)
    {
        std::cerr << "File is empty: " << filename << std::endl;
        return 0; // Return 0 if the file is empty
    }
    file.seekg(0, std::ios::beg); // Reset the file pointer to the beginning

    // Read the checkpoint version (assumes it's stored as a 4-byte integer)
    file.read(reinterpret_cast<char *>(&checkpointVersion), sizeof(checkpointVersion));
    if (file.fail())
    {
        std::cerr << "Failed to read checkpoint version from file: " << filename << std::endl;
        return 0; // Return 0 if reading fails
    }

    file.close();

    return checkpointVersion;
}
