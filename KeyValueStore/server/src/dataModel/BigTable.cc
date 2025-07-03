#include "BigTable.h"
#include "Logger.h"

RowData::RowData()
{
    pthread_mutex_init(&row_mutex, nullptr);
}

RowData::~RowData()
{
    pthread_mutex_destroy(&row_mutex);
}

const unordered_map<string, string> *RowData::getRowPointer() const
{
    return &columns;
}

void RowData::putColumn(const string &column, const string &value)
{
    pthread_mutex_lock(&row_mutex);
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Putting in putColumn" << endl;
    columns[column] = value;
    // printf("Column %s added with value %s\n", column.c_str(), value.c_str());
    pthread_mutex_unlock(&row_mutex);
}

bool RowData::getColumn(const string &column, string &value)
{
    pthread_mutex_lock(&row_mutex); // Lock the mutex before reading
    auto it = columns.find(column);
    if (it != columns.end())
    {
        value = it->second;
        pthread_mutex_unlock(&row_mutex);
        return true;
    }
    pthread_mutex_unlock(&row_mutex);
    return false;
}

bool RowData::delColumn(const string &column)
{
    pthread_mutex_lock(&row_mutex);
    auto it = columns.find(column);
    if (it != columns.end())
    {
        columns.erase(it);
        pthread_mutex_unlock(&row_mutex);
        return true;
    }
    pthread_mutex_unlock(&row_mutex);
    return false;
}

void BigTable::put(const string &rowKey, const string &column, const string &value)
{
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Putting in BigTable" << endl;
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "RowKey: " << rowKey << endl;
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Column: " << column << endl;
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Value: " << value << endl;
    auto it = table.find(rowKey);
    if (it == table.end())
    {
        table[rowKey] = RowData();
        it = table.find(rowKey);
    }
    // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "RowKey: " << rowKey << endl;
    it->second.putColumn(column, value);
}

bool BigTable::get(const string &rowKey, const string &column, string &value)
{
    auto it = table.find(rowKey);
    if (it != table.end())
    {
        return it->second.getColumn(column, value);
    }
    return false;
}

bool BigTable::cput(const string &rowKey, const string &column, const string &value1, const string &value2)
{
    string temp;
    bool find = get(rowKey, column, temp);
    if (find == false || temp != value1)
    {
        return false;
    }
    put(rowKey, column, value2);
    return true;
}

void BigTable::del(const string &rowKey, const string &column)
{
    auto it = table.find(rowKey);
    if (it == table.end())
    {
        return;
    }
    it->second.delColumn(column);
}

unordered_map<string, RowData> *BigTable::getTablePointer()
{
    std::cout << "BigTable::getTablePointer()" << std::endl;
    return &table;
}
