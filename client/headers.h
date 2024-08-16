#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/stat.h>

#define POOL_SIZE 10
#define PIECE_SIZE 1024
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

using namespace std;

extern bool isDevMode;
extern string authToken;

extern map <pair<string, string>, string> fileNameToFilePath; // fileName, groupName, filePath
extern map <string, vector<int>> filePathToAvailablePieces; // filePath, pieceNumber
extern set <pair <string, string>> downloadingFiles; // groupName, fileNmae
extern set <pair <string, string>> downloadedFiles; // groupName, fileName


extern mutex nameToPathMutex, pathToPieceMutex, downloadFileMutex;

// classes.cpp
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

// handlers.cpp
void openSeederSocket(pair <string, int> seederIpPort);
void handleLeecherRequest(int leecherSocket, string leecherIP, int leecherPort);
string executeCommand(string command);
void processUserRequests(int clientSocket, string inputFromClient, pair<string, int> trackerIpPort,  pair<string, int> seederIpPort);
void handleFileDownload(string fileName, string groupName, string destinationPath, int fileSize, string SHAsStr, string ipAndPortsStr, int clientSocket, pair<string, int> trackerIpPort, pair<string, int> seederIpPort);
void downloadFile(string fileName, string groupName, string destinationPath, int fileSize, vector <string> SHAs, unordered_map <int, vector <string>> pieceToSeeders, int clientSocket, pair<string, int> trackerIpPort, pair<string, int> seederIpPort);

// utils.cpp
vector <string> tokenize(string buffer, char separator);
pair<pair <string, int>, pair <string, int> > processArgs(int argc, char *argv[]);
vector<string> findSHA(string filePath);
string findPieceSHA(string pieceData);
int giveFileSize(string filePath);
void raiseError(string errorMessage);
void raiseError(string errorMessage, int errCode);
void raiseWarning(string warningMessage);
void raiseSuccess(string successMessage);