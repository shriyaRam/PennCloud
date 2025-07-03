#ifndef REPLICATION_UTILS_H
#define REPLICATION_UTILS_H
#include <string>
using namespace std;

bool sendToPrimary(const string &row, const string &column, const string &value, const string &primary_server, const string &address)
{
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << absl::StrFormat("In checking primary [PUT] Key: %s, Column: %s, Value: %s", row, column, value);
    if (primary_server == address)
    {
        return true;
    }

    return false;
}
#endif // REPLICATION_UTILS_H