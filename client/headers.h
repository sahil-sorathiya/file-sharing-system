#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h> // for O_RDONLY
#include <errno.h>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/stat.h>

// #include <condition_variable>
// #include <functional>
// #include <atomic>

#define POOL_SIZE 10
#define PIECE_SIZE 1024

using namespace std;

extern map <pair<string, string>, string> fileNameToFilePath; // fileName, groupName, filePath
extern map <pair<string, vector<int> > filePathToAvailablePieces // filePath, pieceNumber
extern mutex nameToPathMutex, pathToPieceMutex;

extern string authToken;
class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueueTask(std::function<void()> task);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

    void workerThread();
};

void processUserRequests(int clientSocket, pair<string, int> trackerIpPort);
vector <string> tokenize(string buffer, char separator);
pair<pair <string, int>, pair <string, int> > processArgs(int argc, char *argv[]);
int giveFileSize(const char *filePath);
vector<string> findSHA(const char* filePath);