#include "headers.h"

void processUserRequests(int clientSocket, pair<string, int> trackerIpPort){
    string inputFromClient;
    cout << "Enter Command : " << flush;
    getline(cin, inputFromClient);

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

    else if(tokens[0] == "upload_file"){

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

        messageForTracker = tokens[0] + " " + fileName + " " + tokens[2] + " " + to_string(fileSize) + " ";
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
        cout << "Error : " + receivedDataTokens[1] + "\n" << flush;
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
    else if(tokens[0] == "upload_file"){
        string filePath = tokens[1];
        vector<string> temp = tokenize(filePath, '/');
        string fileName = temp[temp.size() - 1];
        string groupName = tokens[2];
        fileNameToFilePath[{fileName, groupName}] = filePath;
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

        thread t2(handleFileDownload, fileName, destinationPath, fileSize, SHAsStr, ipAndPortsStr);
        t2.detach();

        return;
    }

    cout << "Success: " + receivedDataTokens[1] + "\n" << flush;
    
    
    return;    
}

void handleFileDownload(string fileName, string destinationPath, int fileSize, string SHAsStr, string ipAndPortsStr){
    vector <string> SHAs = tokenize(SHAs, ':');
    vector <string> ipAndPorts = tokenize(ipAndPorts, ',');

    

}