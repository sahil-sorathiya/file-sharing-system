#include "headers.h"

void openSeederSocket(pair <string, int> seederIpPort){
    int domainAddressFormat = AF_INET; //: Address Family (IPv4)
    int type = SOCK_STREAM; //: TCP
    int protocol = 0; //: TCP/IP

    int seederSocket = socket(domainAddressFormat, type, protocol);
    if(seederSocket < 0){
        cerr << "Error at setsockopt\n" << flush;
        cout << "ERROR : " + to_string(errno) + "\n" << flush;
        exit(errno); //: Exit with the error number
    }

    struct sockaddr_in seederAddr;
    seederAddr.sin_family = AF_INET; //: Address Family (IPv4)
    seederAddr.sin_port = htons(seederIpPort.second); //: Port
    seederAddr.sin_addr.s_addr = htonl(INADDR_ANY); //: IPv4 Address of current computer 

    int opt = 1;
    if (setsockopt(seederSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
        cerr << "Error at setsockopt\n" << flush;
        cout << "ERROR : " + to_string(errno) + "\n" << flush;
        exit(errno);
    }

    if (bind(seederSocket, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
        cerr << "Error binding socket\n" << flush;
        close(seederSocket);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    cout << "Seeder binded at IP " + seederIpPort.first + " and Port " + to_string(seederIpPort.second) + "\n" << flush;
    
    if (listen(seederSocket, 5000) < 0) {
        cerr << "Error listening for connections\n" << flush;
        close(seederSocket);
        cout << "ERROR : " + to_string(errno)  + "\n" << flush;
        exit(errno);
    }

    cout << "Seeder listning on Port " + to_string(seederIpPort.second) + "\n" << flush;
    
    while(true){
        struct sockaddr_in leecherAddr;
        socklen_t leecherAddrSize = sizeof(leecherAddr);
        
        cout << "Waiting for connection\n" << flush;
        int leecherSocket = accept(seederSocket, (struct sockaddr*)&leecherAddr, &leecherAddrSize);
        if (leecherSocket < 0) {
            cerr << "Error accepting leecher connection\n" << flush;
            close(seederSocket);
            cout << "ERROR : " + to_string(errno)  + "\n" << flush;
            exit(errno);
        }
        cout << "Connection established with FD of " + to_string(leecherSocket) + "\n" << flush;

        char leecherIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(leecherAddr.sin_addr), leecherIP, INET_ADDRSTRLEN);
        int leecherPort = ntohs(leecherAddr.sin_port);

        thread t1(handleLeecherRequest, leecherSocket, leecherIP, leecherPort);
        t1.detach();
    }
}

void handleLeecherRequest(int leecherSocket, string leecherIP, int leecherPort){
    while (true) {
        char buffer[10240];
        int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) {
            cout << "Connection Closed: Leecher-socket at fd " + to_string(leecherSocket) + " IP " + leecherIP + " Port " + to_string(leecherPort) + " closed the connection\n" << flush;
            close(leecherSocket);
            break;
        } else if (bytesRead < 0) {
            cerr << "Connection Force-Closed: Error reading from leecherSocket at fd " + to_string(leecherSocket) + " IP " + leecherIP + " Port " + to_string(leecherPort) + "\n" << flush;
            close(leecherSocket);
            break;
        } else {
            // Handle the received data
            string receivedData = string(buffer, bytesRead);

            cout << "Received data: " << receivedData << "\n" << flush;

            string response = "";
            

            if(send(leecherSocket, response.c_str(), response.size(), 0) < 0){
                cerr << "Error sending message to leecher " + string(strerror(errno))  + "\n" << flush;
                close(leecherSocket);
                break;
            }
        }
    }
}

string executeCommand(string command){
    vector <string> tokens = tokenize(command, ' ');
    if(token[0] == "give_piece_info"){
        string fileName = token[1];
        string groupName = token[2];
        
        lock_guard <mutex> guard(nameToPathMutex);
        lock_guard <mutex> guard(pathToPieceMutex);

        if(fileNameToFilePath.find({fileName, groupName}) == fileNameToFilePath.end()){
            return "SeederError: File not Exist!!\n";
        }

        string filePath = fileNameToFilePath[{fileName, groupName}];
        if(filePathToAvailablePieces.find(filePath) == filePathToAvailablePieces.end()){
            return "SeederError: File not Exist!!\n";
        }

        string temp = "SeederSuccess:";
        for(auto it: filePathToAvailablePieces[filePath]){
            temp.append(" " + to_string(it));
        }

        return temp;
    }
    
    if(token[0] == "send_piece"){
        string fileName = token[1];
        string groupName = token[2];
        int pieceNumber = stoi(token[3]);

        lock_guard <mutex> guard(nameToPathMutex);
        lock_guard <mutex> guard(pathToPieceMutex);

        if(fileNameToFilePath.find({fileName, groupName}) == fileNameToFilePath.end()){
            return "SeederError: File not Exist!!\n";
        }

        string filePath = fileNameToFilePath[{fileName, groupName}];
        if(filePathToAvailablePieces.find(filePath) == filePathToAvailablePieces.end()){
            return "SeederError: File not Exist!!\n";
        }

        vector <int> tempVec = filePathToAvailablePieces[filePath];
        if(find(tempVec.begin(), tempVec.end(), pieceNumber) == tempVec.end()){
            return "SeederError: Piece not Found!!\n";
        }

        int fd = open(destinationPath, O_RDONLY, S_IRUSR);
        if (fd == -1) {
            return "SeederError: Failed to open file at Seeder!!\n";
        }

        int pieceOffset = PIECE_SIZE * pieceToSeedersVector[i].first;
        if(lseek(fd, pieceOffset, SEEK_SET) == -1){
            return "SeederError: Failed to Seek at seeder!!\n";
        }

        char buffer[PIECE_SIZE];
        int bytesRead = read(fd, buffer, PIECE_SIZE);
        if(bytesRead == -1){
            return "SeederError: Failed to Read a piece at seeder!!\n";
        }

        if (close(fd) == -1) {
            return "SeederError: Failed to close file at seeder!!\n";
        }

        string pieceData = string(buffer, bytesRead);
        return "SeederSuccess: " + pieceData;
    }

    return "SeederError: Invalid Command!!\n" << flush;
}

void processUserRequests(int clientSocket, string inputFromClient, pair<string, int> trackerIpPort){


    vector <string> tokens = tokenize(inputFromClient, ' ');
    string messageForTracker = "";
    if(tokens.size() == 0) {
        return;
    }
    if(tokens[0] == "quit"){
        cout << "Bye.\n" << flush;
        close(clientSocket);
        exit(0);
    }

    else if(tokens[0] == "show_downloads");

    else if(tokens[0] == "upload_file" || tokens[0] == "upload_file_1"){

        if(tokens.size() != 3) {
            cout << "Error: Invalid Arguments!!";
            return;
        }
        struct stat info;
        string filePath = tokens[1];
        if (stat(filePath.c_str(), &info) != 0 || !(info.st_mode & S_IFREG)) {
            cout << "Error: File Path is Invalid!!";
            return;
        }

        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];

        vector<string> fileSHAs = findSHA(filePath.c_str());
        if(fileSHAs.size() == 0) return;
        int fileSize = giveFileSize(filePath.c_str());

        messageForTracker = "upload_file" + " " + fileName + " " + tokens[2] + " " + to_string(fileSize) + " ";
        for(auto it : fileSHAs) {
            messageForTracker.append(it + ":");
        }
        
        messageForTracker.append(" " + authToken);

    }

    else if(tokens[0] == "download_file"){

        if(tokens.size() != 4) {
            cout << "Error: Invalid Arguments!!";
            return;
        }
        struct stat info;
        string destinationPath = tokens[3];
        if (stat(destinationPath.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
            cout << "Error: Destination Path is Invalid!!";
            return;
        }
        if(destinationPath[destinationPath.size() - 1] != '/') {
            destinationPath += '/';
        }
        string filePath = destinationPath + tokens[2];
        if (stat(filePath.c_str(), &info) == 0 && (info.st_mode & S_IFREG)) {
            cout << "Warning: File Already Exists!! Do you want to replace it? [Y/N]: " << flush;
            string temp;
            getline(cin, temp);
            if(temp == "N") {
                cout << "Enter Different file name: " << flush;
                getline(cin, tokens[2]);
            }
        }

        messageForTracker = tokens[0] + " " + tokens[1] + " " + tokens[2] + " " + authToken;
    }

    else if(tokens[0] == "create_user") messageForTracker = inputFromClient;

    else if(tokens[0] == "login") messageForTracker = inputFromClient;

    else messageForTracker = inputFromClient + " " + authToken;

    cout << "Sending : " + messageForTracker + "\n" << flush;

    if(send(clientSocket, messageForTracker.c_str(), messageForTracker.size(), 0) < 0){
        cerr << "Error: Packet has not been sent to tracker " + string(strerror(errno))  + "\n" << flush;
        close(clientSocket);
        exit(errno);
    }

    char buffer[10240];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == 0) {
        cerr << "Connection Closed: Tracker-socket at fd " + to_string(clientSocket) + " IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + " closed the connection\n" << flush;
        close(clientSocket);
        exit(errno);
    } 
    if (bytesRead < 0) {
        cerr << "Connection Force-Closed: Error reading from clientSocket at fd " + to_string(clientSocket) + " IP " + trackerIpPort.first + " Port " + to_string(trackerIpPort.second) + "\n" << flush;
        close(clientSocket);
        exit(errno);
    } 

    string receivedData = string(buffer, bytesRead);
    vector <string> receivedDataTokens = tokenize(receivedData, ' ');
    
    if(receivedDataTokens[0] == "TrackerError:"){
        cout << "Error : " + receivedData.substr(14) + "\n" << flush;
        return;
    }
    
    if(tokens[0] == "login"){
        authToken = receivedDataTokens[1];
        cout << "Success: User Loggedin Successfully!!\n" << flush;
        return;
    }
    else if(tokens[0] == "logout"){
        authToken = "default";
    }
    else if(tokens[0] == "upload_file" || tokens[0] == "upload_file_1"){
        string filePath = tokens[1];
        int fileSize = giveFileSize(filePath);
        int numOfPieces = fileSize / PIECE_SIZE;


        if(fileSize % PIECE_SIZE) numOfPieces++;

        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];
        string groupName = tokens[2];
        fileNameToFilePath[{fileName, groupName}] = filePath;
        if(tokens[0] == "upload_file") {
            for(int i = 0; i < numOfPieces; i++) {
                filePathToAvailablePieces[filePath].push_back(i);
            }
        }
    }
    else if(tokens[0] == "stop_share"){
        string fileName = tokens[2];
        string groupName = tokens[1];
        fileNameToFilePath.erase({fileName, groupName});
    }
    else if(tokens[0] == "download_file"){
        string fileName = tokens[2];
        string destinationPath = tokens[3];
        int fileSize = stoi(receivedDataTokens[1]);
        string SHAsStr = receivedDataTokens[2];
        string ipAndPortsStr = receivedDataTokens[3];
        if(destinationPath[destinationPath.size() - 1] != '/') {
            destinationPath += '/';
        }
        destinationPath += fileName;
        handleFileDownload(fileName, destinationPath, fileSize, SHAsStr, ipAndPortsStr, clientSocket, trackerIpPort);
        return;
    }

    cout << "Success: " + receivedDataTokens[1] + "\n" << flush;
    
    
    return;    
}

void handleFileDownload(string fileName, string destinationPath, int fileSize, string SHAsStr, string ipAndPortsStr, int clientSocket, pair<string, int> trackerIpPort){
    vector <string> SHAs = tokenize(SHAs, ':');
    vector <string> ipAndPorts = tokenize(ipAndPorts, ',');

    unordered_map <int, vector <int> > pieceToSeeders; // piece no., socket

    for(auto it: ipAndPorts){
        vector <string> temp = tokenize(ipAndPorts, ':');
        string ip = temp[0];
        int port = stoi(temp[1]);

        int totalPieces = SHAs.size()-1;

        int leecherSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (leecherSocket < 0) {
            cerr << "Error: creating socket!!\n" << flush;
            cout << "ERROR : " + to_string(errno)  + "\n" << flush;
            exit(errno);
        }

        struct sockaddr_in seederAddr;
        seederAddr.sin_family = AF_INET;
        seederAddr.sin_port = htons(port);  

        if (inet_pton(AF_INET, ip.c_str(), &seederAddr.sin_addr) <= 0) {
            cerr << "Error: converting IP address of seeder!!\n" << flush;
            cout << "ERROR : " + to_string(errno)  + "\n" << flush;
            close(leecherSocket);
            exit(errno);
        }

        cout << "connecting with seeder at IP " + ip + " Port " + to_string(port) + "\n" << flush;
        if (connect(leecherSocket, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
            cerr << "Error: connecting to seeder!!\n" << flush;
            cout << "ERROR : " + to_string(errno)  + "\n" << flush;
            close(leecherSocket);
            exit(errno);
        }

        cout << "Success: Connected to Seeder!1\n" << flush;

        string messageForSeeder = "give_piece_info " + fileName + " " + groupName; 
        if(send(leecherSocket, messageForSeeder.c_str(), messageForSeeder.size(), 0) < 0){
            cerr << "Error: Packet has not been sent to seeder " + string(strerror(errno))  + "\n" << flush;
            close(leecherSocket);
            exit(errno);
        }

        char buffer[10240];
        int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) {
            cerr << "Connection Closed: Seeder-socket at fd " + to_string(leecherSocket) + " IP " + ip + " Port " + to_string(port) + " closed the connection\n" << flush;
            close(leecherSocket);
            exit(errno);
        } 
        if (bytesRead < 0) {
            cerr << "Connection Force-Closed: Error reading from leecherSocket at fd " + to_string(leecherSocket) + " IP " + ip + " Port " + to_string(port) + "\n" << flush;
            close(leecherSocket);
            exit(errno);
        } 

        string receivedData = string(buffer, bytesRead);
        vector <string> receivedDataTokens = tokenize(receivedData, ' ');
        
        if(receivedDataTokens[0] == "SeederError:"){
            cerr << "Error: " << receivedData.substr(13) << flush;
            return;
        }

        for(int i = 1; i < receivedDataTokens.size(); i++){
            pieceToSeeders[stoi(receivedDataTokens[i])].push_back(leecherSocket);            
        }
    }

    if(pieceToSeeders.size() != totalPieces) {
        cout << "Error: Downloading the file!! All pieces of file is not available in the group!!\n" << flush;
    }

    thread t2(downloadFile, fileName, destinationPath, fileSize, SHAs, pieceToSeeders, clientSocket, trackerIpPort);
    t2.detach();

}

void downloadFile(string fileName, string destinationPath, int fileSize, vector <string> SHAs, unordered_map <int, vector <int>> pieceToSeeders, int clientSocket, pair<string, int> trackerIpPort){
    vector <pair <int, vector <int> > pieceToSeedersVector;

    for(auto it: pieceToSeeders){
        pieceToSeedersVector[it.first].push_back({it.first, it.second});
    }

    sort(pieceToSeedersVector.begin(), pieceToSeedersVector.end(), [](const auto &a, const auto &b) {
        return a.second.size() < b.second.size();
    });

    ThreadPool pool(POOL_SIZE); 
            
    mutex isFirstPieceMutex;

    bool isFirstPiece = true;
    //: Looping for all the pieces
    for (int i = 0; i < pieceToSeedersVector.size(); i++) {
        pool.enqueueTask([i] {
            while(1){

                //: Randomly selecting one Seeder from many
                int n = pieceToSeedersVector[i].second.size();
                srand(time(nullptr)); //: setting a seed
                int random_number = std::rand() % (n);
                leecherSocket = pieceToSeedersVector[i].second[random_number];

                string messageForSeeder = "send_piece " + fileName + " " + groupName + " " + i; 
                if(send(leecherSocket, messageForSeeder.c_str(), messageForSeeder.size(), 0) < 0){
                    cerr << "Error: Packet has not been sent to seeder " + string(strerror(errno))  + "\n" << flush;
                    close(leecherSocket);
                    exit(errno);
                }

                char buffer[PIECE_SIZE];
                int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
                if (bytesRead == 0) {
                    cerr << "Connection Closed: Seeder-socket at fd " + to_string(leecherSocket) + " IP " + ip + " Port " + to_string(port) + " closed the connection\n" << flush;
                    close(leecherSocket);
                    exit(errno);
                } 
                if (bytesRead < 0) {
                    cerr << "Connection Force-Closed: Error reading from leecherSocket at fd " + to_string(leecherSocket) + " IP " + ip + " Port " + to_string(port) + "\n" << flush;
                    close(leecherSocket);
                    exit(errno);
                } 

                string receivedData = string(buffer, bytesRead);
                vector <string> receivedDataTokens = tokenize(receivedData, ' ');

                if(receivedDataTokens[0] == "SeederError:"){
                    cerr << "Error: " + receivedData.substr(13) << flush;
                    continue;
                }

                string pieceData = receivedDataTokens[1];

                string pieceSHA = findPieceSHA(pieceData);

                if(pieceSHA != SHAs[pieceToSeedersVector[i].first + 1]){
                    cerr << "Error: SHA mismatched!!\n" << flush;
                    continue;
                }
        
                int fd = open(destinationPath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                if (fd == -1) {
                    cerr << "Error: Failed to open file!!\n" << flush;
                    return;
                }

                int pieceOffset = PIECE_SIZE * pieceToSeedersVector[i].first;
                if(lseek(fd, pieceOffset, SEEK_SET) == -1){
                    cerr << "Error: Failed to Seek!!\n" << flush;
                    return;
                }

                ssize_t written = write(fd, pieceData, pieceData.size());
                if (written == -1) {
                    cerr << "Error: Failed to write!!\n" << flush;
                    close(fd);
                    return;
                } else if (written != (ssize_t)pieceData.size) {
                    cerr << "Error: Incomplete write!!\n" << flush;
                    close(fd);
                    return;
                }
                {
                    lock_guard <mutex> guard(isFirstPieceMutex);
 
                    if(isFirstPiece) {
                        string command = "upload_file_1 " + destinationPath + " " + groupName;
                        processUserRequests(clientSocket, command, trackerIpPort);
                        isFirstPiece = false;

                    }
                }
                {
                    lock_guard <mutex> guard(pathToPieceMutex);

                    filePathToAvailablePieces[destinationPath].push_back(pieceToSeedersVector[i].first);
                }
                if (close(fd) == -1) {
                    cerr << "Error: Failed to close file!!\n" << flush;
                    return;
                }       
                break;    
            }

        });
    }
    vector <string> tempSHAs = findSHA(destinationPath);
    if(tempSHAs[0] != SHAs[0]){
        cerr << "Error: File SHA mismatched. " << flush;
        if (unlink(destinationPath) == -1) {
            cerr << "\nError: Deleting file.\n" << flush;
            return;
        }
        cerr << "File Deleted!!\n" << flush;
    }

    cout << "Success: File Downloaded Successfully\n" << flush;
}