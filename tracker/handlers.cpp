#include "headers.h"

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

        return userLogin(userName, password, clientIP, clientPort);
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

string userLogin(string userName, string password, string clientIP, int clientPort){
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


