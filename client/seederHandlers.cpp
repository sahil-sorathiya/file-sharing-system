#include "headers.h"

void openSeederSocket(pair <string, int> seederIpPort){
    int domainAddressFormat = AF_INET; //: Address Family (IPv4)
    int type = SOCK_STREAM; //: TCP
    int protocol = 0; //: TCP/IP

    int seederSocket = socket(domainAddressFormat, type, protocol);
    if(seederSocket < 0){
        raiseError("SeederError: Error at setsockopt", errno);
        exit(errno);
    }

    struct sockaddr_in seederAddr;
    seederAddr.sin_family = AF_INET; //: Address Family (IPv4)
    seederAddr.sin_port = htons(seederIpPort.second); //: Port
    seederAddr.sin_addr.s_addr = htonl(INADDR_ANY); //: IPv4 Address of current computer 

    int opt = 1;
    if (setsockopt(seederSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        raiseError("SeederError: Error at setsockopt", errno);
        exit(errno);
    }

    if (bind(seederSocket, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
        raiseError("SeederError: Error binding socket", errno);
        close(seederSocket);
        exit(errno);
    }

    if (listen(seederSocket, 5000) < 0) {
        raiseError("SeederError: Error listening for connections", errno);
        close(seederSocket);
        exit(errno);
    }

    // if(isDevMode) raiseSuccess("SeederSuccess: Seeder listning on Port " + to_string(seederIpPort.second));
    
    while(true){
        struct sockaddr_in leecherAddr;
        socklen_t leecherAddrSize = sizeof(leecherAddr);
        
        int leecherSocket = accept(seederSocket, (struct sockaddr*)&leecherAddr, &leecherAddrSize);
        if (leecherSocket < 0) {
            if(isDevMode) raiseError("SeederError: Error accepting leecher connection", errno);
            close(seederSocket);
            exit(errno);
        }

        char leecherIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(leecherAddr.sin_addr), leecherIP, INET_ADDRSTRLEN);
        int leecherPort = ntohs(leecherAddr.sin_port);

        thread t1(handleLeecherRequest, leecherSocket, leecherIP, leecherPort);
        t1.detach();
    }
}

void handleLeecherRequest(int leecherSocket, string leecherIP, int leecherPort){
    while (true) {
        char buffer[524288];
        int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) {
            close(leecherSocket);
            break;
        } else if (bytesRead < 0) {
            close(leecherSocket);
            break;
        } else {
            string receivedData(buffer, bytesRead);
            string response = executeCommand(receivedData);

            if(send(leecherSocket, response.c_str(), response.size(), 0) < 0){
                close(leecherSocket);
                break;
            }
        }
    }
}

string executeCommand(string command){
    vector <string> tokens = tokenize(command, ' ');
    if(tokens[0] == "give_piece_info"){
        string fileName = tokens[1];
        string groupName = tokens[2];
        
        lock_guard <mutex> guard_1(nameToPathMutex);
        lock_guard <mutex> guard_2(pathToPieceMutex);

        if(fileNameToFilePath.find({fileName, groupName}) == fileNameToFilePath.end()){
            return "SeederSuccess:  \n";
        }

        string filePath = fileNameToFilePath[{fileName, groupName}];
        if(filePathToAvailablePieces.find(filePath) == filePathToAvailablePieces.end()){
            return "SeederSuccess:  \n";
        }

        string temp = "SeederSuccess:";
        for(auto it: filePathToAvailablePieces[filePath]){
            temp.append(" " + to_string(it));
        }

        return temp;
    }
    
    if(tokens[0] == "send_piece"){
        string fileName = tokens[1];
        string groupName = tokens[2];
        int pieceNumber = stoi(tokens[3]);

        lock_guard <mutex> guard_1(nameToPathMutex);
        lock_guard <mutex> guard_2(pathToPieceMutex);

        if(fileNameToFilePath.find({fileName, groupName}) == fileNameToFilePath.end()){
            return "SeederError: File not Exist!!\n";
        }

        string filePath = fileNameToFilePath[{fileName, groupName}];
        if(filePathToAvailablePieces.find(filePath) == filePathToAvailablePieces.end()){
            return "SeederError: Filepiece not Exist!!\n";
        }

        vector <int> tempVec = filePathToAvailablePieces[filePath];
        if(find(tempVec.begin(), tempVec.end(), pieceNumber) == tempVec.end()){
            return "SeederError: Piece not Found!!\n";
        }

        int fd = open(filePath.c_str(), O_RDONLY, S_IRUSR);
        if (fd == -1) {
            return "SeederError: Failed to open file at Seeder!!\n";
        }

        int pieceOffset = PIECE_SIZE * pieceNumber;
        if(lseek(fd, pieceOffset, SEEK_SET) == -1){
            return "SeederError: Failed to Seek at seeder!!\n";
        }

        char buffer[PIECE_SIZE];
        int bytesRead = read(fd, buffer, PIECE_SIZE);
        if(bytesRead == -1){
            return "SeederError: Failed to Read a piece at seeder!!\n";
        }

        close(fd);
        
        string pieceData(buffer, bytesRead);
        string r = "SeederSuccess: " + pieceData;
        return r;
    }

    return "SeederError: Invalid Command!!\n";
}