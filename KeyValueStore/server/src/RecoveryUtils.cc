#include "RecoveryUtils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include "absl/strings/str_format.h"
#include <fcntl.h>  // For open, O_RDONLY, O_WRONLY, etc.
#include <unistd.h> // For close
#include <sys/types.h>
#include "SharedContext.h"
#include "MiscUtils.h"
#include "checkpoint.h"

using namespace std;

CheckPointReply fetchCheckPoint(shared_ptr<SharedContext> sharedContext, int32_t tabletIndex)
{
    CheckPointArgs args;
    args.set_tabletno(tabletIndex);
    CheckPointReply reply;
    grpc::ClientContext clientContext;
    grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->sendCheckPoint(&clientContext, args, &reply);
    logWithTimestamp(sharedContext->getServerManager()->get_address()) << absl::StrFormat("Fetching checkpoint from primary server: %s", sharedContext->getServerManager()->get_primary_server());

    if (!status.ok())
    {
        ABSL_LOG(ERROR) << absl::StrFormat("Failed to fetch checkpoint from primary server. Error: %s (code: %d)", status.error_message(), status.error_code());
        return reply;
    }

    return reply;

    // Fetch the checkpoint from the primary server
}

void processLogFile(const std::string &sourcePath, std::shared_ptr<BigTable> bigTable)
{
    logWithTimestamp(" ") << "Processing log file: " << sourcePath << endl;
    std::ifstream sourceFile(sourcePath);
    if (!sourceFile)
    {
        throw std::runtime_error("Error: Unable to open source file: " + sourcePath);
    }

    std::string line;
    std::string logEntry; // Accumulates lines for a single operation
    while (std::getline(sourceFile, line))
    {
        // Ignore empty lines
        if (line.empty())
        {
            continue;
        }

        // Accumulate log lines until "END" is reached
        if (line == "END")
        {
            std::istringstream logStream(logEntry);
            apply_logs_to_memtable(logStream, bigTable); // Process the accumulated log entry
            logEntry.clear();                            // Reset for the next operation
        }
        else
        {
            logEntry += line + "\n"; // Add the line to the current log entry
        }
    }
}

void readLogRecords(const std::string &filename, std::shared_ptr<BigTable> bigTable)
{
    // Open the file in read-only mode
    int fd = open(filename.c_str(), O_RDONLY);
    cout << "Reading log records from file: " << filename << endl;
    if (fd == -1)
    {
        cout << "Error: Unable to open file for reading in readLogRecords" << endl;
        throw std::runtime_error("Error: Unable to open file for reading");
    }
    cout << "File opened successfully" << flush << endl;

    while (true)
    {
        // Read the 4-byte length
        uint32_t recordLength;
        ssize_t bytesRead = read(fd, &recordLength, sizeof(recordLength));
        if (bytesRead == 0) // End of file
            break;
        if (bytesRead != sizeof(recordLength))
        {
            close(fd);
            throw std::runtime_error("Error: Failed to read record length");
        }

        // Read the 4-byte column count
        uint32_t numColumns;
        bytesRead = read(fd, &numColumns, sizeof(numColumns));
        if (bytesRead != sizeof(numColumns))
        {
            close(fd);
            throw std::runtime_error("Error: Failed to read column count");
        }

        // Read the record of the specified length minus the column count size
        uint32_t dataLength = recordLength - sizeof(numColumns);
        std::string record(dataLength, '\0');
        bytesRead = read(fd, record.data(), dataLength);
        if (bytesRead != static_cast<ssize_t>(dataLength))
        {
            close(fd);
            throw std::runtime_error("Error: Failed to read record data");
        }

        // Parse the record
        size_t pos = 0, nextPos;
        nextPos = record.find(',', pos);
        std::string operation = record.substr(pos, nextPos - pos);

        pos = nextPos + 1;
        nextPos = record.find(',', pos);
        std::string row = record.substr(pos, nextPos - pos);

        pos = nextPos + 1;
        nextPos = record.find(',', pos);
        std::string column = record.substr(pos, nextPos - pos);

        pos = nextPos + 1;
        std::string value1, value2;
        if (numColumns >= 4) // For PUT and CPUT
        {
            value1 = record.substr(pos);
            if (numColumns == 5) // For CPUT
            {
                size_t value2Pos = value1.find_last_of(',');
                value2 = value1.substr(value2Pos + 1);
                value1 = value1.substr(0, value2Pos);
            }
        }

        // Print the record
        if (operation == "PUT")
        {
            bigTable->put(row, column, value1);
            // std::cout << "Operation: PUT, Row: " << row << ", Column: " << column << ", Value: " << value1 << std::endl;
        }
        else if (operation == "CPUT")
        {
            bigTable->cput(row, column, value1, value2);
            // std::cout << "Operation: CPUT, Row: " << row << ", Column: " << column
            //           << ", Value1: " << value1 << ", Value2: " << value2 << std::endl;
        }
        else if (operation == "DELETE")
        {
            bigTable->del(row, column);
            // std::cout << "Operation: DELETE, Row: " << row << ", Column: " << column << std::endl;
        }
        else
        {
            std::cerr << "Error: Unknown operation type: " << operation << std::endl;
        }
    }

    // Close the file
    close(fd);
}

void apply_logs_to_memtable(std::istringstream &logStream, std::shared_ptr<BigTable> bigTable)
{
    std::string serial, operation, row, column, value1, value2;

    std::getline(logStream, serial, ',');
    std::getline(logStream, operation, ',');
    std::getline(logStream, row, ',');
    std::getline(logStream, column, ',');

    logWithTimestamp(" ") << "Applying log entry: " << operation << " " << row << " " << column << " " << endl;
    if (operation == "PUT")
    {
        std::ostringstream remainingContent;
        remainingContent << logStream.rdbuf(); // Reads the entire remaining content of the stream
        value1 = remainingContent.str();
        // logWithTimestamp(" ") << "PUT " << value1 << endl;
        bigTable->put(row, column, value1);
        string temp;
        bigTable->get(row, column, temp);
        // logWithTimestamp(" ") << "Value from table is " << temp << endl;
        // logWithTimestamp(" ") << "Value inserted is " << value1 << endl;
        // check if the value is inserted correctly by comparing the value inserted and the value fetched
        if (temp != value1)
        {
            logWithTimestamp(" ") << "Error: Value not inserted correctly" << endl;
        }
    }
    else if (operation == "CPUT")
    {
        std::getline(logStream, value1, ',');
        std::getline(logStream, value2, ',');
        bigTable->cput(row, column, value1, value2);
    }
    else if (operation == "DELETE")
    {
        bigTable->del(row, column);
    }
    else
    {
        std::cerr << "Error: Unknown operation " << operation << std::endl;
    }
}

void fetchMissingLogsByPosition(const std::string &logFile, std::shared_ptr<SharedContext> sharedContext, int32_t tabletIndex)
{
    int64_t currentPosition = 0;
    bool isDone = false;

    // Read the position of the last line in the local file
    std::ifstream localFile(logFile, std::ios::ate | std::ios::binary);
    if (localFile.is_open())
    {
        currentPosition = localFile.tellg(); // Get the current size of the file
        localFile.close();
    }

    while (!isDone)
    {
        LogPositionRequest request;
        request.set_tabletno(tabletIndex);
        request.set_start_position(currentPosition);

        LogPositionResponse response;
        grpc::ClientContext clientContext;
        grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->getMissingLogsFromPosition(&clientContext, request, &response);

        if (!status.ok())
        {
            std::cerr << "Failed to fetch log chunk: " << status.error_message() << std::endl;
            return;
        }

        // Append the received chunk data to the log file
        std::ofstream logFileStream(logFile, std::ios::app | std::ios::binary);
        if (!logFileStream.is_open())
        {
            throw std::runtime_error("Failed to open log file for appending");
        }
        logFileStream << response.chunk_data();
        logFileStream.close();

        // Update the position and check the is_done flag
        currentPosition = response.next_position();
        isDone = response.is_done();
    }

    std::cout << "Log file transfer complete." << std::endl;
}

// void fetchMissingLogs(const string &logFile, shared_ptr<SharedContext> sharedContext, int32_t tabletIndex)
// {
//     LogLineRequest request;
//     std::ifstream file(logFile);
//     if (!file.is_open())
//     {
//         throw std::runtime_error("Log file not found");
//     }

//     int64_t lastLineNumber = 0;
//     std::string line;
//     while (std::getline(file, line))
//     {
//         lastLineNumber++;
//     }
//     file.close();
//     request.set_tabletno(tabletIndex);
//     request.set_last_line_number(lastLineNumber);
//     LogLineResponse response;
//     grpc::ClientContext clientContext;
//     bool isDone = false;

//     while (!isDone)
//     {
//         grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->getMissingLogs(&clientContext, request, &response);
//         if (!status.ok())
//         {
//             std::cerr << "Failed to fetch logs: " << status.error_message() << std::endl;
//             return;
//         }
//         if (!response.data().empty())
//         {
//             std::ofstream logFileStream(logFile, std::ios::app);
//             if (!logFileStream.is_open())
//             {
//                 throw std::runtime_error("Failed to open log file for appending");
//             }
//             logFileStream << response.data();
//             logFileStream.close();
//         }
//         isDone = response.is_done();
//         if (!isDone)
//         {
//             lastLineNumber += count(response.data().begin(), response.data().end(), '\n');
//             request.set_last_line_number(lastLineNumber);
//         }
//     }

//     // Append received log data to the local log file

//     // if (response.is_done())
//     // {
//     //     std::cout << "Log file transfer complete." << std::endl;
//     // }
// }

void fetchCheckpointFile(const std::string &checkpointFile, shared_ptr<SharedContext> sharedContext, int32_t tabletIndex)
{
    // auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
    // std::unique_ptr<CheckpointSync::Stub> stub = CheckpointSync::NewStub(channel);

    int64_t chunkNumber = 0; // Start with the first chunk
                             // bool firstChunk = true;
    std::ofstream checkpointFileStream(checkpointFile, std::ios::trunc | std::ios::binary);

    while (true)
    {
        CheckpointChunkRequest request;
        request.set_chunk_number(chunkNumber);
        request.set_chunk_size(CHUNK_SIZE);
        request.set_tabletno(tabletIndex);

        CheckpointChunkResponse response;
        grpc::ClientContext context;
        grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->getCheckpointChunk(&context, request, &response);

        if (!status.ok())
        {
            std::cerr << "Failed to fetch checkpoint chunk: " << status.error_message() << std::endl;
            break;
        }

        // Append the received chunk to the local checkpoint file
        // std::ofstream checkpointFileStream(
        //     checkpointFile,
        //     isFirstChunk ? (std::ios::trunc | std::ios::binary) : std::ios::binary);
        if (!checkpointFileStream.is_open())
        {
            throw std::runtime_error("Failed to open checkpoint file for appending");
        }
        checkpointFileStream.write(response.chunk_data().c_str(), response.chunk_data().size());

        // firstChunk = false;

        if (response.is_done())
        {
            std::cout << "Checkpoint file transfer complete." << std::endl;
            break;
        }

        chunkNumber++; // Increment to the next chunk
    }
    checkpointFileStream.close();
}

// Fetching log file data
void fetchLogFile(const std::string &checkpointFile, shared_ptr<SharedContext> sharedContext, int32_t tabletIndex)
{
    // auto channel = grpc::CreateChannel(serverAddress, grpc::InsecureChannelCredentials());
    // std::unique_ptr<CheckpointSync::Stub> stub = CheckpointSync::NewStub(channel);

    int64_t chunkNumber = 0; // Start with the first chunk
                             // bool firstChunk = true;
    std::ofstream checkpointFileStream(checkpointFile, std::ios::trunc | std::ios::binary);

    while (true)
    {
        CheckpointChunkRequest request;
        request.set_chunk_number(chunkNumber);
        request.set_chunk_size(CHUNK_SIZE);
        request.set_tabletno(tabletIndex);

        CheckpointChunkResponse response;
        grpc::ClientContext context;
        grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->getLogChunk(&context, request, &response);

        if (!status.ok())
        {
            std::cerr << "Failed to fetch log chunk: " << status.error_message() << std::endl;
            break;
        }

        // Append the received chunk to the local checkpoint file
        // std::ofstream checkpointFileStream(
        //     checkpointFile,
        //     isFirstChunk ? (std::ios::trunc | std::ios::binary) : std::ios::binary);
        if (!checkpointFileStream.is_open())
        {
            throw std::runtime_error("Failed to open log file for appending");
        }
        checkpointFileStream.write(response.chunk_data().c_str(), response.chunk_data().size());

        // firstChunk = false;

        if (response.is_done())
        {
            std::cout << "Log file transfer complete." << std::endl;
            break;
        }

        chunkNumber++; // Increment to the next chunk
    }
    checkpointFileStream.close();
}

void trim(std::string &s)
{
    // Trim leading whitespace
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    { return !std::isspace(ch); }));

    // Trim trailing whitespace
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
}

// Recovery function to recover the server after a soft kill
void recovery(shared_ptr<SharedContext> sharedContext)
{
    logWithTimestamp(sharedContext->getServerManager()->get_address()) << " sharedContext address in recovery: " << sharedContext.get() << flush << endl;
    logWithTimestamp(sharedContext->getServerManager()->get_address()) << "Starting recovery after kill" << flush << endl;
    grpc::ClientContext clientContext;
    ServerMappingArgs serverMappingArgs;
    ServerMappingReply serverMappingReply;
    string address = sharedContext->getServerManager()->get_address();
    serverMappingArgs.set_ipaddressport(address);
    grpc::Status status = sharedContext->getServerManager()->get_coordinator_stub()->requestServerMapping(&clientContext, serverMappingArgs, &serverMappingReply);
    if (!status.ok())
    {
        logWithTimestamp(sharedContext->getServerManager()->get_address()) << "Failed to get server mapping from coordinator during revival" << endl;
    }
    else
    {
        logWithTimestamp(address) << "Server mapping received from coordinator during revival" << endl;
        string startChar = serverMappingReply.startchar();
        string endChar = serverMappingReply.endchar();
        string primaryServer = serverMappingReply.primaryserver();
        logWithTimestamp(address) << "Primary server is " << primaryServer << flush << endl;
        logWithTimestamp(address) << "Start char is " << startChar[0] << endl;
        logWithTimestamp(address) << "End char is " << endChar[0] << endl;
        sharedContext->getTabletManager()->calculateTabletBoundaries(startChar[0], endChar[0], 3);
        sharedContext->getServerManager()->set_primary_server(primaryServer);
        sharedContext->getServerManager()->set_status(true);
    }

    if (address == sharedContext->getServerManager()->get_primary_server())
    {
        logWithTimestamp(address) << "No need to recover from admin console, I am the primary server" << endl
                                  << flush;
    }
    else
    {

        for (int i = 0; i < 3; i++)
        {
            CheckPointReply checkPointReply = fetchCheckPoint(sharedContext, i);
            filesystem::path currentSnapshotPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(sharedContext->getServerManager()->get_address())) / "snapshots" / (to_string(i) + ".bin");
            filesystem::path currentLogPath = filesystem::current_path() / serverRecoveryPath / (sanitizeAddress(sharedContext->getServerManager()->get_address())) / "logs" / (to_string(i) + ".txt");
            int32_t checkPointVersion = getCheckpointVersion(currentSnapshotPath.string());
            logWithTimestamp(address) << "Checkpoint version after fetching is " << checkPointVersion << endl;
            logWithTimestamp(address) << "Checkpoint version in reply is " << checkPointReply.checkpointversion() << endl;

            if (checkPointReply.checkpointversion() == checkPointVersion)
            {
                fetchMissingLogsByPosition(currentLogPath.string(), sharedContext, i);
            }
            else
            {
                logWithTimestamp(address) << "Checkpoint version is not same as the version in the tablet manager" << endl;
                fetchCheckpointFile(currentSnapshotPath.string(), sharedContext, i);
                fetchLogFile(currentLogPath.string(), sharedContext, i);
                // fetchMissingLogsByPosition(currentLogPath.string(), sharedContext, i);
                sharedContext->getTabletManager()->set_tablet_version(i, checkPointReply.checkpointversion());
                sharedContext->getTabletManager()->set_current_tablet_index(i);
            }
        }
    }
    TabletIndexArgs tabletIndexArgs;
    TabletIndexReply tabletIndexReply;

    if (address == sharedContext->getServerManager()->get_primary_server())
    {
        grpc::ClientContext clientContextForCoordinator;
        AllAddressReply allAddressReply;
        vector<string> allAddresses;
        AllAddressArgs allAddressArgs;
        allAddressArgs.set_ipaddressport(address);
        grpc::Status status = sharedContext->getServerManager()->get_coordinator_stub()->fetchAllAddress(&clientContextForCoordinator, allAddressArgs, &allAddressReply);

        if (!status.ok())
        {
            logWithTimestamp(address) << "Failed to fetch all addresses from coordinator" << endl;
        }
        else
        {
            allAddresses.assign(allAddressReply.addresses().begin(), allAddressReply.addresses().end());
            for (auto server : allAddresses)
            {
                logWithTimestamp(address) << "Adding server " << server << " to server manager" << endl;
                sharedContext->getServerManager()->add_server(server);
            }
            logWithTimestamp(address) << "All addresses received from coordinator" << endl;
        }
    }
    grpc::ClientContext contextForFetchTablet;

    grpc::Status fetchTabletStatus = sharedContext->getServerManager()->get_primary_stub()->fetchCurrentTabletIndex(&contextForFetchTablet, tabletIndexArgs, &tabletIndexReply);

    if (!fetchTabletStatus.ok())
    {
        logWithTimestamp(address) << "Failed to fetch current tablet index from primary server" << endl;
    }
    else
    {
        logWithTimestamp(address) << "Current tablet index is " << tabletIndexReply.tabletindex() << endl;
        sharedContext->getTabletManager()->set_current_tablet_index(tabletIndexReply.tabletindex());
        grpc::ClientContext tempClientContext;
        LoadTabletArgs loadTabletArgs;
        LoadTabletReply loadTabletReply;
        loadTabletArgs.set_tabletno(tabletIndexReply.tabletindex());
        // create stub for current server and then call loadNewTablet
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        auto stub = InterServer::NewStub(channel);
        grpc::Status loadStatus = stub->loadNewTablet(&tempClientContext, loadTabletArgs, &loadTabletReply);
        if (!loadStatus.ok())
        {
            ABSL_LOG(ERROR) << "Failed to load tablet: " << endl;
        }
        // grpc::Status status = sharedContext->getServerManager()->get_primary_stub()->loadNewTablet(&tempClientContext, loadTabletArgs, &loadTabletReply);
    }
}
