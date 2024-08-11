#include "headers.h"

void handleTrackerQuit(int trackerFd) {
    string cmd;
    while(1) {
        cin >> cmd;
        if(cmd == "quit") {
            close(trackerFd);
            exit(0);
        }
    }
}

void handleClientRequest(int clientSocket, string clientIP, int clientPort){
    while (true) {
            char buffer[10240];
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead == 0) {
                cout << "Connection Closed: Client-socket at fd " + to_string(clientSocket) + " IP " + clientIP + " Port " + to_string(clientPort) + " closed the connection\n" << flush;
                close(clientSocket);
                break;
            } else if (bytesRead < 0) {
                cerr << "Connection Force-Closed: Error reading from clientSocket at fd " + to_string(clientSocket) + " IP " + clientIP + " Port " + to_string(clientPort) + "\n" << flush;
                close(clientSocket);
                break;
            } else {
                // Handle the received data
                string receivedData = string(buffer, bytesRead);

                cout << "Received data: " << receivedData << "\n" << flush;

                string response = executeCommand(clientSocket, clientIP, clientPort, receivedData);

                if(send(clientSocket, response.c_str(), response.size(), 0) < 0){
                    cerr << "Error sending message to client " + string(strerror(errno))  + "\n" << flush;
                    close(clientSocket);
                    break;
                }
            }
            
        }
}

string executeCommand(int clientSocket, string clientIP, int clientPort, string command){
    if(command == "") return "TrackerError: Invalid Command!!";
    vector <string> tokens = tokenize(command, ' ');
    cout << tokens.size() << endl;
    
    if(tokens.size() < 1) return "TrackerError: Invalid Command!!";

    if(tokens[0] == "create_user"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to create_user command!!";
        
        string userName = tokens[1];
        string password = tokens[2];

        return createUser(userName, password);
    }

    if(tokens[0] == "login"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to login command!!";
        
        string userName = tokens[1];
        string password = tokens[2];

        return login(userName, password, clientIP, clientPort);
    }

    if(tokens[0] == "create_group") {
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to create_group command!!";
        string groupName = tokens[1];
        string authToken = tokens[2];
        return createGroup(groupName, authToken);
    }

    if(tokens[0] == "join_group"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to join_group command!!";
        string groupName = tokens[1];
        string authToken = tokens[2];
        return joinGroup(groupName, authToken);
    }

    if(tokens[0] == "list_requests"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to list_requests command!!";
        string groupName = tokens[1];
        string authToken = tokens[2];
        return listRequests(groupName, authToken);
    }

    if(tokens[0] == "list_groups"){
        if(tokens.size() != 2) return "TrackerError: Invalid Arguments to list_groups command!!";
        string authToken = tokens[1];
        return listGroups(authToken);
    }

    if(tokens[0] == "accept_request"){
        if(tokens.size() != 4) return "TrackerError: Invalid Arguments to accept_request command!!";
        string groupName = tokens[1];
        string userName = tokens[2];
        string authToken = tokens[3];
        return acceptRequest(groupName, userName, authToken);
    }

    if(tokens[0] == "list_files"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to list_files command!!";
        string groupName = tokens[1];
        string authToken = tokens[2];
        return listFiles(groupName, authToken);
    }

    if(tokens[0] == "upload_file"){
        if(tokens.size() != 6) return "TrackerError: Invalid Arguments to list_files command!!";
        string fileName = tokens[1];
        string groupName = tokens[2];
        string fileSize = tokens[3];
        string SHAs = tokens[4];
        string authToken = tokens[5];
        return uploadFiles(fileName, groupName, fileSize, SHAs, authToken);
    }

    if(tokens[0] == "download_file"){
        if(tokens.size() != 4) return "TrackerError: Invalid Arguments to download_file command!!";
        string groupName = tokens[1];
        string fileName = tokens[2];
        string authToken = tokens[3];
        return downloadFile(fileName, groupName, authToken);
    }

    if(tokens[0] == "stop_share"){
        if(tokens.size() != 4) return "TrackerError: Invalid Arguments to stop_share command!!";
        string groupName = tokens[1];
        string fileName = tokens[2];
        string authToken = tokens[3];
        return stopShare(groupName, fileName, authToken);
    }
    
    if(tokens[0] == "leave_group"){
        if(tokens.size() != 3) return "TrackerError: Invalid Arguments to leave_group command!!";
        string groupName = tokens[1];
        string authToken = tokens[2];
        return leaveGroup(groupName, authToken);
    }

    if(tokens[0] == "logout"){
        if(tokens.size() != 2) return "TrackerError: Invalid Arguments to logout command!!";
        string authToken = tokens[1];
        return logout(authToken);
    }

    return "TrackerError: Invalid Command!!";
}

string createUser(string userName, string password){
    lock_guard <mutex> guard(userMapMutex);
    if(registeredUsers.find(userName) != registeredUsers.end()) {
        return "TrackerError: Username Already Exist!!";
    }
    registeredUsers[userName] = User(userName, password);
    return "TrackerSuccess: User Registered Sucessfully!!";
}

string login(string userName, string password, string clientIP, int clientPort){
    {
        lock_guard <mutex> guard(userMapMutex);

        if(registeredUsers.find(userName) == registeredUsers.end()) {
            return "TrackerError: Username Not Found!!";
        }
        if(registeredUsers[userName].password != password) {
            return "TrackerError: Incorrect Password!!";
        }
    }
    string payload = userName;
    string loginToken = generateToken(payload);
    
    {
        lock_guard <mutex> guard(loginMutex);
        userToIp[userName] = clientIP + ":" + to_string(clientPort);
    }
    return "TrackerSuccess: " + loginToken;
}

string createGroup(string groupName, string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";
    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) != groups.end()) {
            return "TrackerError: Group Already Exist!!";
        }

        Group group(groupName, userName);
        groups[groupName] = group;

        return "TrackerSuccess: Group Successfully Created!!";
    }
}

string joinGroup(string groupName, string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";
    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it != group.participants.end()){
            return "TrackerError: You are already a member of this group!!";
        }

        if(group.pendingJoins.find(userName) != group.pendingJoins.end()){
            return "TrackerError: Joining request has already been sent!!";
        }

        groups[groupName].pendingJoins.insert(userName);
        
        return "TrackerSuccess: Joining request has been raised!!";
    }
}

string listRequests(string groupName, string authToken){
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";
    
    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group not Exist!!";
        }

        Group group = groups[groupName];
        
        if(group.participants[0] != userName){
            return "TrackerError: You are not the admin of this group!!";
        }

        string temp = "";
        for(auto it: group.pendingJoins) temp.append("\n" + it);
        return "TrackerSuccess: " + temp;
    }
}

string listGroups(string authToken){
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);
        string temp = "";
        for(auto it: groups) temp.append("\n" + it.first);
        return "TrackerSuccess: " + temp;
    }
}

string acceptRequest(string groupName, string pendingUserName, string authToken){
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);

        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }
        Group group = groups[groupName];

        if(group.participants[0] != userName){
            return "TrackerError: You are not an admin of the group!!";
        }

        if(group.pendingJoins.find(pendingUserName) == group.pendingJoins.end()){
            return "TrackerError: Username not exist in a pending requests list!!";
        }

        groups[groupName].pendingJoins.erase(pendingUserName);
        groups[groupName].participants.push_back(pendingUserName);

        return "TrackerSuccess: Member Added to the Group";
    }
}

string listFiles(string groupName, string authToken){
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";
    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it == group.participants.end()){
            return "TrackerError: You are not a member of this group!!";
        }

        string temp = "";
        for(auto it: group.files) temp.append("\n" + it.first);

        return "TrackerSuccess: " + temp;
    }
}


string uploadFiles(string fileName, string groupName, string fileSize, string SHAs, string authToken){
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it == group.participants.end()){
            return "TrackerError: You are not a member of this group!!";
        }

        vector <string> SHAVector = tokenize(SHAs, ':');

        int sizeOfSHAVector = (stoi(fileSize) / PIECE_SIZE) + 1; // +1 for entire file's SHA
        if(stoi(fileSize) % PIECE_SIZE) sizeOfSHAVector++;

        if((int)SHAVector.size() != sizeOfSHAVector + 1) {
            return "TrackerError: Invalid (More/Less) number of SHAs!!";
        }

        // If file exist in group Check SHA
        if(group.files.find(fileName) != group.files.end()){
            if(group.files[fileName].SHA[0] != SHAVector[0]){
                return "TrackerError: File with same name but different content exist, change name of the file!!";
            }

            // If sha matches insert userName into set
            groups[groupName].files[fileName].userNames.insert(userName);
            return "TrackerSuccess: File Uploaded Successfully!!";
        }

        File newFile(fileName, SHAVector, stoi(fileSize), userName);
        groups[groupName].files[fileName] = newFile;
        return "TrackerSuccess: File Uploaded Successfully!!";

    }
}

string downloadFile(string fileName, string groupName, string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);

        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it == group.participants.end()){
            return "TrackerError: You are not a member of this group!!";
        }

       
        if(group.files.find(fileName) == group.files.end()){
            return "TrackerError: File Not Found!!";
        }

        File file = group.files[fileName];
        string temp = "";
        
        temp.append(to_string(file.size) + " ");

        for(auto it : file.SHA) {
            temp.append(it + ":");
        }

        {
            lock_guard <mutex> guard(loginMutex);
            for(auto it : file.userNames) {
                if(userToIp.find(it) != userToIp.end()) {
                    temp.append(userToIp[it] + ",");
                }
            }
        }
        return "TrackerSuccess: " + temp;
    }
}

string stopShare(string groupName, string fileName, string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it == group.participants.end()){
            return "TrackerError: You are not a member of this group!!";
        }

        if(group.files.find(fileName) == group.files.end()){
            return "TrackerError: File doesn't exist!!";
        }

        if(group.files[fileName].userNames.find(userName) == group.files[fileName].userNames.end()){
            return "TrackerError: You are not sharing this file!!";
        }

        groups[groupName].files[fileName].userNames.erase(userName);
        return "TrackerSuccess: File Sharing is stopped!!";

    }
}

string leaveGroup(string groupName, string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    {
        lock_guard <mutex> guard(groupMapMutex);
        if(groups.find(groupName) == groups.end()) {
            return "TrackerError: Group Not Exist!!";
        }

        Group group = groups[groupName];
        
        auto it = find(group.participants.begin(), group.participants.end(), userName);
        if(it == group.participants.end()){
            return "TrackerError: You are not a member of this group!!";
        }

        groups[groupName].participants.erase(it);

        // If Only 1 participant is there
        if(groups[groupName].participants.size()==0) {
            groups.erase(groupName);
            return "TrackerSuccess: You Left The Group Successfully!!";
        }

        for(auto &it : groups[groupName].files) {
            it.second.userNames.erase(userName);
        }
        return "TrackerSuccess: You Left The Group Successfully!!";
    }
}

string logout(string authToken) {
    string userName = validateToken(authToken);
    if(userName == "") return "TrackerError: Authentication Failed!!";

    lock_guard <mutex> guard(loginMutex);
    userToIp.erase(userName);
    return "TrackerSuccess: User Logged Out Successfully!!";
}

