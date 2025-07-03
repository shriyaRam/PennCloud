#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <unordered_map>
#include "BigTable.h"
#include <memory>

// Decode a 32-byte length into an integer
uint32_t decodeLength(const std::ifstream &file);

// Read a fixed number of bytes from a file
std::string readBytes(std::ifstream &file, size_t numBytes);

// Load data from a file into BigTable
void loadTableToBigTable(const std::string &filename, std::shared_ptr<BigTable> bigTable);

// Encode an integer length into a 32-byte representation (uint32_t)
std::vector<uint8_t> encodeLength(uint32_t length);

// Encode a string into a byte representation
std::vector<uint8_t> encodeString(const std::string &str);

// Write raw bytes to a file
void writeBytes(std::ofstream &file, const std::vector<uint8_t> &bytes);

// Serialize a row into a binary file
void serializeRow(const std::string &rowKey,
                  const std::unordered_map<std::string, std::string> &columns,
                  const std::string &outputFile);

// Serialize the entire BigTable into a binary file
void serializeBigTable(std::shared_ptr<BigTable> bigTable, const std::string &outputFile, int32_t checkpointVersion);

int getCheckpointVersion(const std::string &filename);

#endif // CHECKPOINT_H
