#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <string>
#include <grpcpp/grpcpp.h>
#include "proto/interserver.grpc.pb.h"
#include "proto/interserver.pb.h"

void constructUpdateArgs(UpdateArgs &updateArgs, const std::string &row, const std::string &column, const std::string &value1, const std::string &value2, const std::string &operation);

std::string sanitizeAddress(const std::string &address);

#endif // MISC_UTILS_H
