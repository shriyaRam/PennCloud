#include "MiscUtils.h"
#include <algorithm> // For std::replace
#include <grpcpp/grpcpp.h>
#include "proto/interserver.grpc.pb.h"
#include "proto/interserver.pb.h"
void constructUpdateArgs(UpdateArgs &updateArgs, const std::string &row, const std::string &column,
                         const std::string &value1, const std::string &value2, const std::string &operation)
{
    updateArgs.set_row(row);
    updateArgs.set_column(column);
    updateArgs.set_value1(value1);
    updateArgs.set_value2(value2);
    updateArgs.set_operation(operation);
}

std::string sanitizeAddress(const std::string &address)
{
    std::string sanitized = address;
    // Replace invalid characters (e.g., ':') with '_'
    std::replace(sanitized.begin(), sanitized.end(), ':', '_');
    return sanitized;
}
