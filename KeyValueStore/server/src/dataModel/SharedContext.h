// SharedContext.h
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "BigTable.h"
#include "ServerManager.h"
#include "TabletManager.h"
#include "Logger.h"
#include <pthread.h>

using namespace std;

class SharedContext
{
private:
    // shared_ptr<BigTable> table_;
    shared_ptr<ServerManager> serverManager_;
    shared_ptr<TabletManager> tabletManager_;
    unordered_map<int32_t, unique_ptr<pthread_mutex_t>> snapshotMutexes_;
    unordered_map<int32_t, unique_ptr<pthread_mutex_t>> logFileMutexes_;
    unordered_map<int32_t, unique_ptr<pthread_mutex_t>> replicationLogMutexes_;
    unique_ptr<pthread_mutex_t> currentTabletIndexMutex_;

    // void initializeMutexes(const vector<filesystem::path> &filePaths, unordered_map<string, unique_ptr<pthread_mutex_t>> &mutexMap)
    // {
    //     for (const auto &filePath : filePaths)
    //     {
    //         auto mutex = std::make_unique<pthread_mutex_t>();
    //         pthread_mutex_init(mutex.get(), nullptr);
    //         mutexMap[filePath] = std::move(mutex);
    //     }
    // }

public:
    // shared_ptr<BigTable> getTable() { return table_; }
    // void emptyTableMemory() { table_.reset(); }
    // shared_ptr<BigTable> getTable() { return table_; }
    // void emptyTableMemory() { table_.reset(); }
    shared_ptr<ServerManager>
    getServerManager()
    {
        return serverManager_;
    }
    // void createTable() { table_ = std::make_shared<BigTable>(); }
    // void createTable() { table_ = std::make_shared<BigTable>(); }

    SharedContext(std::shared_ptr<ServerManager> serverManager, std::shared_ptr<TabletManager> tabletManager)
        : serverManager_(move(serverManager)), tabletManager_(move(tabletManager))
    {

        for (int32_t i = 0; i < 3; i++)
        {
            snapshotMutexes_[i] = std::make_unique<pthread_mutex_t>();
            pthread_mutex_init(snapshotMutexes_[i].get(), nullptr);
            logFileMutexes_[i] = std::make_unique<pthread_mutex_t>();
            pthread_mutex_init(logFileMutexes_[i].get(), nullptr);
            replicationLogMutexes_[i] = std::make_unique<pthread_mutex_t>();
            pthread_mutex_init(replicationLogMutexes_[i].get(), nullptr);
        }
        currentTabletIndexMutex_ = std::make_unique<pthread_mutex_t>();
        // initializeMutexes(snapshotPaths, snapshotMutexes_);
        // initializeMutexes(logPaths, logFileMutexes_);
    }

    ~SharedContext() = default;
    // {
    //     // for (auto &pair : snapshotMutexes_)
    //     // {
    //     //     pthread_mutex_destroy(pair.second.get());
    //     // }
    //     // for (auto &pair : logFileMutexes_)
    //     // {
    //     //     pthread_mutex_destroy(pair.second.get());
    //     // }
    // }

    void put(const string &row, const string &column, const string &value)
    {
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Current tablet is " << tabletManager_->get_current_tablet() << endl;
        tabletManager_->get_current_tablet()->put(row, column, value);
    }

    bool get(const string &row, const string &column, string &value)
    {
        return tabletManager_->get_current_tablet()->get(row, column, value);
    }

    bool cput(const string &row, const string &column, const string &value1, const string &value2)
    {
        return tabletManager_->get_current_tablet()->cput(row, column, value1, value2);
        return tabletManager_->get_current_tablet()->cput(row, column, value1, value2);
    }

    void del(const string &row, const string &column)
    {
        tabletManager_->get_current_tablet()->del(row, column);
    }

    std::shared_ptr<TabletManager> getTabletManager()
    {
        return tabletManager_;
    }

    pthread_mutex_t *getSnapshotMutex(int32_t tabletNo)
    {
        return snapshotMutexes_[tabletNo].get();
    }

    pthread_mutex_t *getLogFileMutex(int32_t tabletNo)
    {
        return logFileMutexes_[tabletNo].get();
    }

    pthread_mutex_t *getCurrentTabletIndexMutex()
    {
        return currentTabletIndexMutex_.get();
    }

    // void setCoordinatorServer(string coordinatorAddress)
    // {
    //     coordinatorChannel_ = grpc::CreateChannel(coordinatorAddress, grpc::InsecureChannelCredentials());
    //     coordinatorStub_ = CoordinatorService::NewStub(coordinatorChannel_);
    // }
};

#endif // SHARED_CONTEXT_H
