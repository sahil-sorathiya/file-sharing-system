#include "headers.h"

//: This function is used for process & send commands given by User from std input
//TODO Also it is used for "upload_file_on_first_piece" by downloadFile() function, when first piece of file is downloaded
void processUserRequests(int clientSocket, string inputFromClient, pair<string, int> trackerIpPort, pair<string, int> seederIpPort){

    //: Tokenizing command given by User
    vector <string> tokens = tokenize(inputFromClient, ' ');
    string messageForTracker = "";
    
    //: If there is no command, do nothing, get ready for another input
    if(tokens.size() == 0) {
        return;
    }

    //: if command is "quit", close the connection with tracker and exit the program
    if(tokens.size() == 1 && tokens[0] == "quit"){
        raiseSuccess("Bye");
        close(clientSocket);
        exit(0);
    }

    //: if command is "show_downloads" print download status of files.
    else if(tokens[0] == "show_downloads"){
        lock_guard <mutex> guard(downloadFileMutex);
        for(auto it: downloadingFiles){
            cout << "[D][" + it.first + "] " + it.second + "\n" << flush; 
        }
        for(auto it: downloadedFiles){
            cout << "[C][" + it.first + "] " + it.second + "\n" << flush; 
        }
        cout << "D (Downloading), C (Complete)\n" << flush; 
        return;
    }

    //: if command is "upload_file" build message for tracker with fileName, SHAs etc.
    else if(tokens[0] == "upload_file"){
        if(tokens.size() != 3) {
            raiseError("ClientError: Invalid Arguments to upload_file command");
            return;
        }
        
        string filePath = tokens[1];
        string groupName = tokens[2];
        
        //: Validating path of a file
        struct stat info;
        if (stat(filePath.c_str(), &info) != 0 || !(info.st_mode & S_IFREG)) {
            raiseError("ClientError: File Path is Invalid", errno);
            return;
        }

        //: Extracting fileName form filePath
        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];

        //: Store whole File SHA, and piecewise SHAs in one vector
        vector<string> fileSHAs = findSHA(filePath.c_str());
        if(fileSHAs.size() == 0) return;

        //: Store size of a file
        int fileSize = giveFileSize(filePath.c_str());
        if(fileSize == -1) {
            return;
        }

        //: Builiding Command for tracker
        //: "upload_file fileName groupName fileSize (whole_file_SHA:piece_1_SHA:...:piece_N_SHA) authToken"
        messageForTracker = "upload_file " + fileName + " " + groupName + " " + to_string(fileSize) + " ";
        for(auto it : fileSHAs) {
            messageForTracker.append(it + ":");
        }
        messageForTracker.append(" " + authToken);

    }

    //: This command is not for User's use
    //: This command is executed by "downloadFile()" function 
    //: When first piece of any file is downloaded, we need to tell tracker that
    //TODO This user also has file now
    //: ASSUMPTION: if any client has atleast one piece of the file, tracker will save info that this client has file
    else if(tokens[0] == "upload_file_on_first_piece") {
        string fileName = tokens[1];
        string groupName = tokens[2];
        string fileSize = tokens[3];
        string fileSHAs = tokens[4];

        //: Builiding Command for tracker
        //: "upload_file fileName groupName fileSize (whole_file_SHA:piece_1_SHA:...:piece_N_SHA) authToken"
        messageForTracker = "upload_file " + fileName + " " + groupName + " " + fileSize + " " + fileSHAs + " " + authToken;
    }

    else if(tokens[0] == "download_file"){
        string groupName = tokens[1];
        string fileName = tokens[2];
        string destinationPath = tokens[3];

        if(tokens.size() != 4) {
            raiseError("ClientError: Invalid Arguments to download_file command");
            return;
        }

        struct stat info;
        if (stat(destinationPath.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
            raiseError("ClientError: Destination Path is Invalid", errno);
            return;
        }

        if(destinationPath[destinationPath.size() - 1] != '/') {
            destinationPath += '/';
        }

        string filePath = destinationPath + fileName;
        if (stat(filePath.c_str(), &info) == 0 && (info.st_mode & S_IFREG)) {
            raiseWarning("ClientWarning: File Already Exists");
            while(1){
                cout << "Do you want to overwrite it? [Y/N]: ";
                
                string temp;
                getline(cin, temp);
                
                if(temp == "N") return;
                if(temp == "Y") break;
            }
        }

        messageForTracker = "download_file " + groupName + " " + fileName + " " + authToken;
    }

    else if(tokens[0] == "create_user") messageForTracker = inputFromClient;

    else if(tokens[0] == "login") messageForTracker = inputFromClient + " " + seederIpPort.first + ":" + to_string(seederIpPort.second);

    else messageForTracker = inputFromClient + " " + authToken;

    if(send(clientSocket, messageForTracker.c_str(), messageForTracker.size(), 0) < 0){
        raiseError("ClientError: Packet has not been sent to tracker", errno);
        close(clientSocket);
        exit(errno);
    }

    char buffer[524288];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == 0) {
        raiseWarning("ClientWarning : Tracker closed the connection\n");
        close(clientSocket);
        exit(errno);
    } 
    if (bytesRead < 0) {
        raiseError("ClientError: Error reading from clientSocket!! Connection Force-Closed", errno);
        close(clientSocket);
        exit(errno);
    } 

    string receivedDataFromTracker(buffer, bytesRead);
    vector <string> receivedDataTokens = tokenize(receivedDataFromTracker, ' ');
    
    if(receivedDataTokens[0] == "TrackerError:"){
        raiseError(receivedDataFromTracker);
        return;
    }
    
    if(tokens[0] == "login"){
        authToken = receivedDataTokens[1];
        raiseSuccess("ClientSuccess : Logged in Successfully");
        return;
    }

    if(tokens[0] == "logout"){
        authToken = "default";
    }

    if(tokens[0] == "upload_file"){
        string filePath = tokens[1];
        
        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];
        string groupName = tokens[2];
        
        fileNameToFilePath[{fileName, groupName}] = filePath;
        
        int fileSize = giveFileSize(filePath);
        if(fileSize == -1){
            return;
        }
        int numOfPieces = fileSize / PIECE_SIZE;

        if(fileSize % PIECE_SIZE) numOfPieces++;

        for(int i = 0; i < numOfPieces; i++) {
            filePathToAvailablePieces[filePath].push_back(i);
        }
    }

    if(tokens[0] == "upload_file_on_first_piece"){
        string filePath = tokens[1];
        
        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];
        string groupName = tokens[2];
        
        fileNameToFilePath[{fileName, groupName}] = filePath;
        return;
    }

    if(tokens[0] == "stop_share"){
        string fileName = tokens[2];
        string groupName = tokens[1];
        fileNameToFilePath.erase({fileName, groupName});
    }

    if(tokens[0] == "download_file"){
        string groupName = tokens[1];
        string fileName = tokens[2];
        string destinationPath = tokens[3];

        int fileSize = stoi(receivedDataTokens[1]);
        string SHAsStr = receivedDataTokens[2];
        string ipAndPortsStr = receivedDataTokens[3];

        if(destinationPath[destinationPath.size() - 1] != '/') {
            destinationPath += '/';
        }

        destinationPath += fileName;
        handleFileDownload(fileName, groupName, destinationPath, fileSize, SHAsStr, ipAndPortsStr, clientSocket, trackerIpPort, seederIpPort);
        return;
    }

    raiseSuccess("ClientSuccess: " + receivedDataFromTracker.substr(strlen("TrackerSuccess: ")));
    return;
}

void handleFileDownload(string fileName, string groupName, string destinationPath, int fileSize, string SHAsStr, string ipAndPortsStr, int clientSocket, pair<string, int> trackerIpPort, pair<string, int> seederIpPort){
    vector <string> SHAs = tokenize(SHAsStr, ':');
    vector <string> ipAndPorts = tokenize(ipAndPortsStr, ',');
    
    int totalPieces = SHAs.size()-1;
    raiseWarning("TotaoPieces" + to_string(totalPieces));
    unordered_map <int, vector <string> > pieceToSeeders; // piece no., ip:port

    for(auto it: ipAndPorts){
        vector <string> temp = tokenize(it, ':');
        string ip = temp[0];
        int port = stoi(temp[1]);

        

        int leecherSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (leecherSocket < 0) {
            if(isDevMode) raiseError("LeecherError: Leecher-Socket not created", errno);
            continue;
        }

        struct sockaddr_in seederAddr;
        seederAddr.sin_family = AF_INET;
        seederAddr.sin_port = htons(port);  

        if (inet_pton(AF_INET, ip.c_str(), &seederAddr.sin_addr) <= 0) {
            if(isDevMode) raiseError("LeecherError: Error converting IP address of Seeder", errno);
            close(leecherSocket);
            continue;
        }

        if (connect(leecherSocket, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
            if(isDevMode) raiseError("LeecherError: Error connecting to Seeder", errno);
            close(leecherSocket);
            continue;
        }

        string messageForSeeder = "give_piece_info " + fileName + " " + groupName; 
        if(send(leecherSocket, messageForSeeder.c_str(), messageForSeeder.size(), 0) <= 0){
            if(isDevMode) raiseError("LeecherError: Packet has not been sent to seeder");
            close(leecherSocket);
            continue;
        }

        char buffer[524288];
        int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) {
            if(isDevMode) raiseWarning("ClientWarning : Seeder closed the connection\n");
            close(leecherSocket);
            continue;
        } 
        if (bytesRead < 0) {
            if(isDevMode) raiseError("ClientError: Error reading from leecherSocket!! Connection Force-Closed", errno);
            close(leecherSocket);
            continue;
        }

        string receivedDataFromSeeder(buffer, bytesRead);
        vector <string> receivedDataTokens = tokenize(receivedDataFromSeeder, ' ');
        
        if(receivedDataTokens[0] == "SeederError:"){
            if(isDevMode) raiseError(receivedDataFromSeeder);
            continue;
        }

        for(int i = 1; i < (int)receivedDataTokens.size(); i++){
            pieceToSeeders[stoi(receivedDataTokens[i])].push_back(it);            
        }
        close(leecherSocket);
    }

    if((int)pieceToSeeders.size() != totalPieces) {
        raiseError("LeecherError: Can not download the file!! All pieces of file is not available in the group");
        cout << "!!\n" << flush;
    }
    {
        lock_guard <mutex> guard(downloadFileMutex);
        downloadingFiles.insert({groupName, fileName});
    }

    thread t2(downloadFile, fileName, groupName, destinationPath, fileSize, SHAs, pieceToSeeders, clientSocket, trackerIpPort, seederIpPort);
    t2.detach();

}

void downloadFile(string fileName, string groupName, string destinationPath, int fileSize, vector <string> SHAs, unordered_map <int, vector <string>> pieceToSeeders, int clientSocket, pair<string, int> trackerIpPort, pair<string, int> seederIpPort){
    vector <pair <int, vector <string>>> pieceToSeedersVector;

    for(auto it: pieceToSeeders){
        pieceToSeedersVector.push_back({it.first, it.second});
    }

    sort(pieceToSeedersVector.begin(), pieceToSeedersVector.end(), [](const auto &a, const auto &b) {
        return a.second.size() < b.second.size();
    });

    // sort(pieceToSeedersVector.begin(), pieceToSeedersVector.end(), [](const auto &a, const auto &b) {
    //     if(a.second.size() != b.second.size())  return a.second.size() < b.second.size();
    //     return a.first < b.first;
    // });

    ThreadPool pool(POOL_SIZE); 
            
    mutex isFirstPieceMutex;
    bool isFirstPiece = true;

    for (int i = 0; i < (int)pieceToSeedersVector.size(); i++) {
        pool.enqueueTask([i, pieceToSeedersVector, SHAs, clientSocket, trackerIpPort, seederIpPort,
        fileName, groupName, &isFirstPiece, &isFirstPieceMutex, destinationPath, fileSize] {
            int retry = 5; //: If any error int below code, we will retry 
            while(1){
                if(retry == 0) {
                    if(isDevMode) raiseError("LeecherError: Packet download failed");
                    break;
                }
                //: Randomly selecting one Seeder from many
                int n = pieceToSeedersVector[i].second.size();
                srand(time(nullptr)); //: setting a seed
                int random_number = std::rand() % (n);
                                
                // int leecherSocket = pieceToSeedersVector[i].second[random_number];
                
                string temp = pieceToSeedersVector[i].second[random_number];
                string ip = "";
                for(auto it : temp) {
                    if(it!=':') ip.push_back(it);
                    else break;
                }

                int port = stoi(temp.substr(ip.size()+1));

                int leecherSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (leecherSocket < 0) {
                    continue;
                }

                struct sockaddr_in seederAddr;
                seederAddr.sin_family = AF_INET;
                seederAddr.sin_port = htons(port);  

                if (inet_pton(AF_INET, ip.c_str(), &seederAddr.sin_addr) <= 0) {
                    close(leecherSocket);
                    continue;
                }

                if (connect(leecherSocket, (struct sockaddr*)&seederAddr, sizeof(seederAddr)) < 0) {
                    close(leecherSocket);
                    continue;
                }


                string messageForSeeder = "send_piece " + fileName + " " + groupName + " " + to_string(pieceToSeedersVector[i].first); 
                
                if(send(leecherSocket, messageForSeeder.c_str(), messageForSeeder.size(), 0) < 0){
                    if(isDevMode) raiseError("LeecherError: Packet has not been sent to seeder");
                    close(leecherSocket);
                    retry--;
                    continue;
                }
                char buffer[PIECE_SIZE + 25];
                int bytesRead = recv(leecherSocket, buffer, sizeof(buffer), 0);
                if (bytesRead == 0) {
                    // if(isDevMode) raiseWarning("ClientWarning : Seeder closed the connection\n");
                    close(leecherSocket);
                    retry--;
                    continue;
                }
                if (bytesRead < 0) {
                    // if(isDevMode) raiseError("ClientError: Error reading from leecherSocket!! Connection Force-Closed", errno);
                    close(leecherSocket);
                    retry--;
                    continue;
                }
                cout << "SOCKET FD : " + to_string(leecherSocket) + "\n" << flush;
                close(leecherSocket);
                string receivedDataFromSeeder(buffer, bytesRead);
                vector <string> receivedDataTokens = tokenize(receivedDataFromSeeder, ' ');
                if(receivedDataTokens[0] == "SeederError:"){
                    // if(isDevMode) raiseError(receivedDataFromSeeder);
                    retry--;
                    continue;
                }
                string pieceData = receivedDataFromSeeder.substr(strlen("SeederSuccess: "));
                string pieceSHA = findPieceSHA(pieceData);
                if(pieceSHA != SHAs[pieceToSeedersVector[i].first + 1]){
                    // if(isDevMode) raiseError("LeecherError: PieceSHA mismatched!!");
                    // break;
                    retry--;
                    continue;
                }
                int fd = open(destinationPath.c_str(), O_WRONLY | O_CREAT , S_IRUSR | S_IWUSR);
                if (fd == -1) {
                    // if(isDevMode) raiseError("LeecherError: Failed to open file");
                    retry--;
                    continue;
                }
                int pieceOffset = PIECE_SIZE * pieceToSeedersVector[i].first;
                if(lseek(fd, pieceOffset, SEEK_SET) == -1){
                    // if(isDevMode) raiseError("LeecherError: Failed to Seek");
                    retry--;
                    continue;
                }
                int written = write(fd, pieceData.c_str(), pieceData.size());

                if (written == -1) {
                    // if(isDevMode) raiseError("LeecherError: LeecherError: Failed to write");
                    retry--;
                    continue;
                }
                if (written != (int)pieceData.size()) {
                    // if(isDevMode) raiseError("LeecherError: Incomplete write");
                    retry--;
                    continue;
                }
                {
                    lock_guard <mutex> guard(isFirstPieceMutex);
                    if(isFirstPiece) {
                        string command = "upload_file_on_first_piece " + fileName + " " + groupName + " " + to_string(fileSize) + " ";
                        for(auto it : SHAs) {
                            command.append(it + ":");
                        }
                        processUserRequests(clientSocket, command, trackerIpPort, seederIpPort);
                        isFirstPiece = false;

                    }
                }
                if (close(fd) == -1) {
                    if(isDevMode) raiseWarning("LeecherError: Failed to close file");
                }
                {
                    lock_guard <mutex> guard(pathToPieceMutex);
                    filePathToAvailablePieces[destinationPath].push_back(pieceToSeedersVector[i].first);
                }
                // close(leecherSocket);
                break;    
            }

        });
    }
    // vector <string> tempSHAs = findSHA(destinationPath);
    // if(tempSHAs[0] != SHAs[0]){
    //     // if(isDevMode) raiseError("LeecherError: File SHA mismatched!! Deleting the file");
    //     if (unlink(destinationPath.c_str()) == -1) {
    //         // if(isDevMode) raiseError("LeecherError: Error deleting file");
    //         return;
    //     }
    //     // raiseSuccess("LeecherSuccess: File Deleted");
    // }
    {
        lock_guard <mutex> guard(downloadFileMutex);
        downloadingFiles.erase({groupName, fileName});
        downloadedFiles.insert({groupName, fileName});
    }
    // raiseSuccess("LeecherSuccess: File Downloaded Successfully");
    return;
}