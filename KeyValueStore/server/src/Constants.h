#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

using namespace std;

inline const string PUT = "PUT";
inline const string DELETE = "DELETE";
inline const string CPUT = "CPUT";
inline const string ROLLBACK = "ROLLBACK";
inline const string FAILURE = "FAILURE";
inline const string SUCCESS = "SUCCESS";
inline const string CHECKPOINT_DONE = "CHECKPOINT_DONE";
inline const string serverRecoveryPath = "../../server/src/checkpoints/";
inline const string logsPath = "../../server/src/checkpoints/logs/";
inline const string snapshotPath = "../../server/src/checkpoints/snapshots/";
inline const int numOfWrites = 50;
inline const string coordinatorAddress = "127.0.0.1:50050";
inline const int64_t CHUNK_SIZE = 3.8 * 1024 * 1024;
inline const string COMMIT = "COMMIT";
inline const string ABORT = "ABORT";

#endif // CONSTANTS_H
