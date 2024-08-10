#include "headers.h"

//: Declaration
// class File {
// public:
    
//     File(string fileName, vector <string> SHA, int size);

//     string fileName;
//     vector <string> SHA; // At zero SHA of entire file, then PieceWise SHA
//     int size;
// };

File::File(string fileName, vector <string> SHA, int size, string userName){
    this->fileName = fileName;
    this->SHA = SHA;
    this->size = size;
    this->userNames.insert(userName);
}

// class User {
// public:

//     User(string userName, string password);

//     string userName;
//     string password;
//     unordered_set <string> groups; // groupname

// };

User::User(string userName, string password){
    this->userName = userName;
    this->password = password;
}

// class Group {
// public:

//     Group(string groupName);

//     string groupName;
//     vector <string> participants; // username
//     unordered_set <string> pendingJoins;  // username
//     unordered_map <string, unordered_set<string>> files; // filename, username
// };

Group::Group(string groupName, string admin) {
    this->groupName = groupName;
    this->participants.push_back(admin);
}

