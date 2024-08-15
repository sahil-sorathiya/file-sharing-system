#include "headers.h"

string authToken = "default";

map <pair<string, string>, string> fileNameToFilePath;
map <string, vector<int> > filePathToAvailablePieces;
set <pair <string, string>> downloadingFiles;
set <pair <string, string>> downloadedFiles;

mutex nameToPathMutex, pathToPieceMutex, downloadFileMutex;

int main(int argc, char *argv[]){
    pair <pair<string, int>, pair<string, int>> clientAndTrackerIpPort = processArgs(argc, argv);
    pair<string, int> seederIpPort = clientAndTrackerIpPort.first;
    pair<string, int> trackerIpPort = clientAndTrackerIpPort.second;

    thread t0(openSeederSocket, seederIpPort);
    t0.detach();

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error: creating socket!!\n" << flush;
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    //: An Internet Address (Socket Address) is represented by sockaddr_in structure
    //: Here we are prepareing Tracker's Internet Address
    struct sockaddr_in trackerAddr;
    trackerAddr.sin_family = AF_INET;
    trackerAddr.sin_port = htons(trackerIpPort.second);  
    
    //: Here we have IP address of tracker, If we have domain-name then first we need to convert it to IP
    //: Using getAddrInfo(string domainName, string Port/Protocol, struct addrinfo options, struct addrinfo *result)
    //: then we can use *result to get IP from struct addrinfo
    //: Here we have IP in string format so we can not use htonl
    //: Instead we use inet_pton(), converting it into uint_32t
    if (inet_pton(AF_INET, trackerIpPort.first.c_str(), &trackerAddr.sin_addr) <= 0) {
        cerr << "Error: converting IP address of tracker!!\n" << flush;
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        close(clientSocket);
        exit(errno);
    }

    cout << "connecting with tracker at IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + "\n" << flush;
    if (connect(clientSocket, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        cerr << "Error: connecting to tracker!!\n" << flush;
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        close(clientSocket);
        exit(errno);
    }
    cout << "Success: Connected to Tracker!!\n" << flush;

    while (true){
        string inputFromClient;
        cout << "Enter Command : " << flush;
        getline(cin, inputFromClient);
        processUserRequests(clientSocket, inputFromClient, trackerIpPort, seederIpPort);
    }

    return 0;
}