#include "headers.h"

int trackerNumber = 1;

int main(int argc, char *argv[]){
    pair <pair<string, int>, pair<string, int>> clientAndTrackerIpPort = processArgs(argc, argv);
    pair<string, int> clientIpPort = clientAndTrackerIpPort.first;
    pair<string, int> trackerIpPort = clientAndTrackerIpPort.second;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "LEECHER : Error creating socket at tracker connect\n" << flush;
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
        cerr << "LEECHER : Error converting IP address of tracker\n" << flush;
        close(clientSocket);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    cout << "LEECHER : connecting with tracker at IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + "\n" << flush;
    if (connect(clientSocket, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        cerr << "LEECHER : Error connecting to tracker\n" << flush;
        close(clientSocket);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }
    cout << "LEECHER : connected\n" << flush;

    char buffer[10240];
    while (true){
        string inputFromLeecher;
        getline(cin, inputFromLeecher);
        if(inputFromLeecher == "quit"){
            close(clientSocket);
            cout << "Bye.\n" << flush;
            exit(0);
        }
        if(send(clientSocket, inputFromLeecher.c_str(), inputFromLeecher.size(), 0) < 0){
            cerr << "LEECHER : Error sending message to tracker " + string(strerror(errno))  + "\n" << flush;
        }

        int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) {
            cout << "LEECHER : Connection Closed: Tracker-socket at fd " + to_string(clientSocket) + " IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + " closed the connection\n" << flush;
            close(clientSocket);
            break;
        } else if (bytesRead < 0) {
            cerr << "LEECHER : Connection Force-Closed: Error reading from clientSocket at fd " + to_string(clientSocket) + " IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + "\n" << flush;
            close(clientSocket);
            break;
        } else {
            string receivedData = string(buffer, bytesRead);
            cout << "LEECHER : Received data: " << receivedData << "\n" << flush;
        }
    }

    return 0;
}