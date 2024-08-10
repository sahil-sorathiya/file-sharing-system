#include "headers.h"

unordered_map <string, User> registeredUsers; // userName
// unordered_map <string, string> tokensToUser; //token, userName
// unordered_map <string, pair<string, string>> UserToIpAndTokens; // userName, IP:PORT, token
unordered_map <string, string> userToIp; // userName, IP:PORT
unordered_map <string, Group> groups; // groupName
mutex userMapMutex, groupMapMutex, loginMutex;

int main(int argc, char *argv[]){

    pair <string, int> trackerIpPort = processArgs(argc, argv);

    //: Much like everything in UNIX sockets are treated as file
    //: To create a socket, we need <sys/socket.h> header file
    int domainAddressFormat = AF_INET; //: Address Family (IPv4)
    int type = SOCK_STREAM; //: TCP
    int protocol = 0; //: TCP/IP

    //: socket() return file descriptor for that created socket
    int trackerFd = socket(domainAddressFormat, type, protocol);
    if(trackerFd < 0){
        cerr << "Error at setsockopt\n" << flush;
        cout << "ERROR : " + to_string(errno) + "\n" << flush;
        exit(errno); //: Exit with the error number
    }

    //: Network Protocol uses Big-Endian Format for communication
    //: But different systems can have different fotmat specifications
    //: To get Big-Endian Format at the end, not depending on which particular Format you system is using
    //: We have some functions in <arpa/inet.h> library
    //: uint_32t htonl(uint32_t hostint32) // Translate 4 Byte int to network format (host to network long)
    //: uint_16t htons(uint16_t hostint16) // Translate 2 Byte int to network format (host to network short)
    //: uint_32t ntohl(uint32_t hostint32) // Translate 4 Byte int to host format (host to network long)
    //: uint_16t ntohs(uint16_t hostint16) // Translate 2 Byte int to host format (host to network short)

    //: An Internet Address (Socket Address) is represented by following structure
    //: struct sockaddr_in {
    //:     sa_family_t sin_family;
    //:     in_port_t sin_port;
    //:     struct in_addr sin_addr;
    //: }

    struct sockaddr_in trackerAddr;
    trackerAddr.sin_family = AF_INET; //: Address Family (IPv4)
    trackerAddr.sin_port = htons(trackerIpPort.second); //: Port
    trackerAddr.sin_addr.s_addr = htonl(INADDR_ANY); //: IPv4 Address of current computer 
    //: In above line we can set IP address also using inet_pton() function

    int opt = 1;
    if (setsockopt(trackerFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        cerr << "Error at setsockopt\n" << flush;
        cout << "ERROR : " + to_string(errno) + "\n" << flush;
        exit(errno);
    }

    //: Binding socket with specific port
    //: If not binded it can associate the socket to any port
    if (bind(trackerFd, (struct sockaddr*)&trackerAddr, sizeof(trackerAddr)) < 0) {
        cerr << "Error binding socket\n" << flush;
        close(trackerFd);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    cout << "Tracker binded at IP " + trackerIpPort.first + " and Port " + to_string(trackerIpPort.second) + "\n" << flush;
    
    //: listen returns fd
    //: We listen on a socket that has been bound with bind
    //: At a time we are allowing 100 connections waiting to be accepted in queue
    //: If queue is full the server will reject additional requests
    if (listen(trackerFd, 5000) < 0) {
        cerr << "Error listening for connections\n" << flush;
        close(trackerFd);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    cout << "Tracker listning on Port " + to_string(trackerIpPort.second) + "\n" << flush;
    
    thread t0(handleTrackerQuit, trackerFd);
    t0.detach();

    while(true){
        struct sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        
        cout << "Waiting for connection\n" << flush;
        //: It accepts incoming connection requests from queue
        int clientSocket = accept(trackerFd, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket < 0) {
            cerr << "Error accepting client connection\n" << flush;
            close(trackerFd);
            cout << "ERROR : " + to_string(errno)  + "\n" << flush;
            exit(errno);
        }
        cout << "Connection established with FD of " + to_string(clientSocket) + "\n" << flush;

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);

        thread t1(handleClientRequest, clientSocket, clientIP, clientPort);
        t1.detach();
    }

    return 0;
}