#include <bits/stdc++.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for O_RDONLY
#include <errno.h>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#define TOKEN_EXPIRY_DURATION 3600
#define SECRET_KEY "chin_tapak_dum_dum"
#define PIECE_SIZE 1024

using namespace std;

extern mutex userMapMutex, groupMapMutex, loginMutex;


class File {
public:
    
    File() = default;
    File(string fileName, vector <string> SHA, int size, string userName);

    string fileName;
    vector <string> SHA; // At zero SHA of entire file, then PieceWise SHA
    int size;
    unordered_set <string> userNames; 
};

class User {
public:
    User() = default;
    User(string userName, string password);

    string userName;
    string password;
    unordered_set <string> groups; // groupName

};

class Group {
public:
    Group() = default;
    Group(string groupName, string admin);

    string groupName;
    vector <string> participants; // username
    unordered_set <string> pendingJoins;  // username
    unordered_map <string, File> files; // filename
};

extern unordered_map <string, User> registeredUsers; // userName
extern unordered_map <string, string> userToIp; // userName, IP:PORT
extern unordered_map <string, Group> groups; // groupName

// utils.cpp
vector <string> tokenize(string buffer, char separator);
pair <string, int> processArgs(int argc, char *argv[]);
string generateToken(string payload);
string validateToken(string token);

// handlers.cpp
void handleTrackerQuit(int trackerFd);
void handleClientRequest(int clientSocket);
string executeCommand(int clientSocket, string command);

string createUser(string userName, string password);
string login(string userName, string password, string seederIpPort);
string createGroup(string groupName, string authToken);
string joinGroup(string groupName, string authToken);
string listRequests(string groupName, string authToken);
string listGroups(string authToken);
string acceptRequest(string groupName, string pendingUserName, string authToken);
string listFiles(string groupName, string authToken);
string uploadFiles(string fileName, string groupName, string fileSize, string SHAs, string authToken);
string downloadFile(string fileName, string groupName, string authToken);
string stopShare(string groupName, string fileName, string authToken);
string leaveGroup(string groupName, string authToken);
string logout(string authToken);



