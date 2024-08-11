#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h> // for O_RDONLY
#include <errno.h>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/stat.h>

#define PIECE_SIZE 1024

using namespace std;

extern map <pair<string, string>, string> fileNameToFilePath; // fileName, groupName, filePath

void processUserRequests(int clientSocket, pair<string, int> trackerIpPort);
vector <string> tokenize(string buffer, char separator);
pair<pair <string, int>, pair <string, int> > processArgs(int argc, char *argv[]);
int giveFileSize(const char *filePath);
vector<string> findSHA(const char* filePath);
extern string authToken;