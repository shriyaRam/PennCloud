#ifndef RECOVERY_UTILS_H
#define RECOVERY_UTILS_H

#include <string>
#include <memory>
#include "SharedContext.h"
#include "Logger.h"
#include "InterServerLogic.h"
#include "Constants.h"

class BigTable;

CheckPointReply fetchCheckPoint(std::shared_ptr<SharedContext> sharedContext, int32_t tabletIndex);
void processLogFile(const std::string &sourcePath, std::shared_ptr<BigTable> bigTable);
void fetchMissingLogsByPosition(const std::string &logFile, std::shared_ptr<SharedContext> sharedContext, int32_t tabletIndex);
void fetchCheckpointFile(const std::string &checkpointFile, std::shared_ptr<SharedContext> sharedContext, int32_t tabletIndex);
void apply_logs_to_memtable(std::istringstream &logStream, std::shared_ptr<BigTable> bigTable);
void readLogRecords(const std::string &filename, std::shared_ptr<BigTable> bigTable);
void fetchLogFile(const std::string &checkpointFile, shared_ptr<SharedContext> sharedContext, int32_t tabletIndex);
void trim(std::string &s);
void recovery(shared_ptr<SharedContext> sharedContext);
#endif // RECOVERY_UTILS_H