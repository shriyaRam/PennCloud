#include <iostream>
#include <unordered_map>
#include <string>
#include "BigTable.h"
#include <memory>
#include "Logger.h"

using namespace std;

class TabletManager
{

public:
    TabletManager()
    {
        currentTabletIndex = -1;
        currentTablet = make_shared<BigTable>();
        // initialize 3 empty tablets in the beginning
        // for (int32_t i = 0; i < 3; i++)
        // {
        //     tablets[i] = make_shared<BigTable>();
        // }
    }

    void set_current_tablet(shared_ptr<BigTable> tablet)
    {
        currentTablet = tablet;
    }

    void set_current_tablet_index(int index)
    {
        currentTabletIndex = index;
    }

    shared_ptr<BigTable> get_current_tablet()
    {
        return currentTablet;
    }

    int32_t get_current_tablet_index()
    {
        return currentTabletIndex;
    }

    int32_t get_tablet_version(int32_t tabletIndex)
    {
        return tabletVersion[tabletIndex];
    }

    void set_tablet_version(int32_t tabletIndex, int32_t version)
    {
        tabletVersion[tabletIndex] = version;
    }

    int fetchTabletIndexOfKey(const std::string &key)
    {
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Key is " << key << endl;
        // for (auto boundary : boundaries)
        // {
        //     logWithTimestamp(sharedContext_->getServerManager()->get_address()) << boundary << endl;
        // }
        logWithTimestamp(" ") << "Inside fetch tablet index " << key << " first char is " << boundaries.front() << " and " << boundaries.back() << endl;
        if (key.empty())
        {
            throw std::invalid_argument("Key cannot be empty");
        }

        char firstChar = tolower(key[0]); // Ensure lowercase for consistent mapping
        if (firstChar < boundaries.front() || firstChar >= boundaries.back())
        {
            throw std::invalid_argument("Key is out of range");
        }

        // Determine the bucket based on the boundaries
        for (size_t i = 0; i < boundaries.size() - 1; ++i)
        {
            if (firstChar >= boundaries[i] && firstChar < boundaries[i + 1])
            {
                return i;
            }
        }

        logWithTimestamp(" ") << "Key: " << key << " First char: " << firstChar << " Boundaries: " << boundaries.front() << " " << boundaries.back() << endl;

        throw std::runtime_error("Unexpected error in tablet assignment");
    }

    shared_ptr<BigTable> get_tablet(int32_t index)
    {
        return tablets[index];
    }

    void set_tablet(int32_t index, shared_ptr<BigTable> tablet)
    {
        tablets[index] = tablet;
    }

    void put(const string &row, const string &column, const string &value)
    {
        // logWithTimestamp(sharedContext_->getServerManager()->get_address()) << "Putting in current tablet" << endl;
        currentTablet->put(row, column, value);
    }

    bool get(const string &row, const string &column, string &value)
    {
        return currentTablet->get(row, column, value);
    }

    bool cput(const string &row, const string &column, const string &value1, const string &value2)
    {
        return currentTablet->cput(row, column, value1, value2);
    }

    void del(const string &row, const string &column)
    {
        currentTablet->del(row, column);
    }

    void emptyTableMemory()
    {
        currentTablet.reset();
    }

    void emptyTableMemory(int32_t index)
    {
        tablets[index].reset();
    }

    void calculateTabletBoundaries(char startChar, char endChar, int numTablets)
    {
        startChar = tolower(startChar);
        endChar = tolower(endChar);
        if (startChar > endChar || numTablets <= 0)
        {
            throw std::invalid_argument("Invalid range or number of tablets");
        }

        int rangeSize = endChar - startChar + 1; // Total number of characters in the range
        int bucketSize = rangeSize / numTablets; // Number of characters per bucket
        int remainder = rangeSize % numTablets;  // Remaining characters to distribute

        boundaries.push_back(startChar);

        char currentBoundary = startChar;
        for (int i = 0; i < numTablets; ++i)
        {
            int bucketRange = bucketSize + (i < remainder ? 1 : 0); // Distribute remainder evenly
            currentBoundary += bucketRange;
            boundaries.push_back(currentBoundary);
        }
    }

    bool clearTablets()
    {
        cout << "Clearing tablets" << endl;
        for (int i = 0; i < 3; i++)
        {
            cout << "Clearing tablet " << i << flush << endl;
            tablets[i].reset();
        }
        return true;
    }

    vector<char> boundaries;

private:
    unordered_map<int32_t, shared_ptr<BigTable>>
        tablets;
    unordered_map<int32_t, int32_t> tabletVersion;
    // initializing to -1
    int32_t currentTabletIndex = -1;
    shared_ptr<BigTable> currentTablet;
};