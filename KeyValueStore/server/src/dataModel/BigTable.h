#ifndef BIGTABLE_H
#define BIGTABLE_H

#include <unordered_map>
#include <string>
#include <pthread.h>
#include <iostream>
using namespace std;

class RowData
{
public:
    RowData();
    ~RowData();

    void putColumn(const string &column, const string &value);
    bool getColumn(const string &column, string &value);
    bool delColumn(const string &column);

    const unordered_map<string, string> *getRowPointer() const;


private:
    unordered_map<string, string> columns;
    pthread_mutex_t row_mutex;
};

class BigTable
{
public:
    void put(const string &rowKey, const string &column, const string &value);
    bool get(const string &rowKey, const string &column, string &value);
    bool cput(const string &rowKey, const string &column, const string &value1, const string &value2);
    void del(const string &rowKey, const string &column);

    unordered_map<string, RowData> *getTablePointer();

private:
    unordered_map<string, RowData> table; // Table: RowKey -> RowData
};

#endif // BIGTABLE_H
