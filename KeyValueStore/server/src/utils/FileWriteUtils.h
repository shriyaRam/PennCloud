#ifndef FILE_WRITE_UTILS_H
#define FILE_WRITE_UTILS_H

#include <string>
#include "SharedContext.h"

/**
 * @brief Logs an operation to the specified file.
 *
 * @param filename The name of the file to log the operation.
 * @param operation The type of operation (PUT, DELETE, CPUT).
 * @param row The row key associated with the operation.
 * @param column The column key associated with the operation.
 * @param value1 The first value for the operation (optional for DELETE).
 * @param value2 The second value for the operation (only used for CPUT).
 */
void logOperation(const std::string &filename, const std::string &operation, const std::string &row,
                  const std::string &column, const std::string &value1 = "", const std::string &value2 = "", const int serial = 0);

/**
 * @brief Updates the snapshot file for the given SharedContext.
 *
 * @param filename The name of the snapshot file to update.
 * @param context The SharedContext object containing the BigTable state.
 */
void updateSnapshot(const std::string &filename, SharedContext &context);

/**
 * @brief Deletes the last line of the specified file efficiently.
 *
 * @param filename The name of the file to truncate.
 */
void deleteLastLineEfficiently(const std::string &filename);

void overwriteFile(const std::filesystem::path &sourcePath, const std::filesystem::path &targetPath);
void clearFile(const std::filesystem::path &filePath);
void updateLineCount(const std::string &metaFile, size_t lineCount);
size_t getCurrentLineCount(const std::string &metaFile);

void logTransaction(const std::string &filename, const int serial, const std::string &operation);
void writeLogRecord(const std::string &filename, const std::string &operation, const std::string &row,
                    const std::string &column, const std::string &value1 = "", const std::string &value2 = "");

#endif // FILE_WRITE_UTILS_H
