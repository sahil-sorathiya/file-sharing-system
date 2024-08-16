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
#define PIECE_SIZE 10
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
extern mutex leecherLoggerMutex, seederLoggerMutex;

extern pair<string, int> seederIpPort, trackerIpPort;

// classes.cpp
class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueueTask(std::function<void()> task);
    void wait();

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable waitCondition;

    std::atomic<bool> stop;
    std::atomic<int> activeTasks;

    void workerThread();
};

// handlers.cpp
void openSeederSocket();
void handleLeecherRequest(int leecherSocket, string leecherIP, int leecherPort);
string executeCommand(string command);
void processUserRequests(int clientSocket, string inputFromClient);
void handleFileDownload(string fileName, string groupName, string destinationPath, int fileSize, string SHAsStr, string ipAndPortsStr, int clientSocket);
void downloadFile(string fileName, string groupName, string destinationPath, int fileSize, vector <string> SHAs, unordered_map <int, vector <string>> pieceToSeeders, int clientSocket);

// utils.cpp
vector <string> tokenize(string buffer, char separator);
void processArgs(int argc, char *argv[]);
void configureLogger();
void leecherLog(string type, string content);
void seederLog(string type, string content);
vector<string> findSHA(string filePath);
string findPieceSHA(string pieceData);
int giveFileSize(string filePath);
void raiseError(string errorMessage);
void raiseError(string errorMessage, int errCode);
void raiseWarning(string warningMessage);
void raiseSuccess(string successMessage);