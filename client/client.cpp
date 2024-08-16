#include "headers.h"

string authToken = "default";
bool isDevMode = false;

map <pair<string, string>, string> fileNameToFilePath;
map <string, vector<int> > filePathToAvailablePieces;
set <pair <string, string>> downloadingFiles;
set <pair <string, string>> downloadedFiles;

mutex nameToPathMutex, pathToPieceMutex, downloadFileMutex;
mutex leecherLoggerMutex, seederLoggerMutex;
pair<string, int> seederIpPort, trackerIpPort;

int main(int argc, char *argv[]){
    //: Processing arguments and extracting Tracker & Seeder IP:Port
    //: This function will set global variables "isDevMode", "seederIpPort", "trackerIpPort"
    processArgs(argc, argv);

    if(isDevMode) configureLogger();
    
    //: Thread for Seeder Listening
    thread t0(openSeederSocket);
    t0.detach();

    //: Client Socket for tracker connection
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        raiseError("ClientError: Client-Socket not created", errno);
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
        raiseError("ClientError: Error converting IP address of Tracker", errno);
        close(clientSocket);
        exit(errno);
    }

    //: Connecting to tracker
    if (connect(clientSocket, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        raiseError("ClientError: Error connecting to Tracker", errno);
        close(clientSocket);
        exit(errno);
    }
    raiseSuccess("ClientSuccess: Connected to Tracker");
    
    while (true){
        string inputFromClient;
        cout << "Enter Command : " << flush;
        getline(cin, inputFromClient);
        processUserRequests(clientSocket, inputFromClient);
    }

    return 0;
}