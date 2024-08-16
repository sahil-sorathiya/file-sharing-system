#include "headers.h"


vector <string> tokenize(string buffer, char separator){
    vector <string> ans;
    string temp;
    for(auto it: buffer){
        if(it == separator) {
            if(temp.size()) ans.push_back(temp);
            temp.clear();
        }
        else temp.push_back(it);
    }
    if(temp.size()) ans.push_back(temp);
    return ans;
}

void processArgs(int argc, char *argv[]){
    if(argc != 3){
        if(argc == 4 && string(argv[3]) == "dev") {
            isDevMode = true;
        }
        else{
            cout << "Invalid arguments!!\n" << flush;
            exit(0);
        }
    }
    
    char *seederIpAndPort = argv[1];
    vector <string> seederIpPortVec = tokenize(seederIpAndPort, ':');
    string seederIp = seederIpPortVec[0];
    int seederPort = stoi(seederIpPortVec[1]);
    
    char *trackerInfoFileName = argv[2];
    int fd = open(trackerInfoFileName, O_RDONLY);
    if(fd < 0) {
        string s = trackerInfoFileName;
        cout << "Error opening " + s + " file\n" << flush;
        exit(errno); 
    }
    
    char buffer[524288];
    if(read(fd, buffer, sizeof(buffer)) < 0) {
        string s = trackerInfoFileName;
        cout << "Error reading " + s + " file\n" << flush;
        exit(errno); 
    }
    close(fd);

    vector <string> ipAndPorts = tokenize(buffer, '\n');
    string trackerIp = ipAndPorts[0];
    int trackerPort = stoi(ipAndPorts[1]);

    //: Setting up global variables
    seederIpPort = {seederIp, seederPort};
    trackerIpPort = {trackerIp, trackerPort};
}

void configureLogger(){
    string logDirPath = "./logs_"+seederIpPort.first+":"+to_string(seederIpPort.second);
    struct stat info;
    if (stat(logDirPath.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        if (mkdir(logDirPath.c_str(), 0755) != 0) {
            raiseError("ClientError: Error making new directory for log");
            return;
        }
    }
    string leecherLogFilePath = logDirPath+"/leecherLogs.txt";
    int fd = open(leecherLogFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd <= 0){
        raiseError("ClientError: Error opening leecher log file");
    }
    string temp = "LEECHER LOGGER FILE";
    int bytesWritten = write(fd, temp.c_str(), temp.size());
    if(bytesWritten < 0){
        raiseError("ClientError: Error writing leecher log file");
    }
    close(fd);
    
    string seederLogFilePath = logDirPath+"/seederLogs.txt";
    fd = open(seederLogFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd <= 0){
        raiseError("ClientError: Error opening leecher log file");
    }
    temp = "SEEDER LOGGER FILE";
    bytesWritten = write(fd, temp.c_str(), temp.size());
    if(bytesWritten < 0){
        raiseError("ClientError: Error writing leecher log file");
    }
    close(fd);
    raiseSuccess("ClientSuccess: Log files created successfully");
    return;
}

void leecherLog(string type, string content){
    lock_guard <mutex> guard(leecherLoggerMutex);
    string logDirPath = "./logs_"+seederIpPort.first+":"+to_string(seederIpPort.second);
    string filePath = logDirPath + "/leecherLogs.txt";

    time_t current_time = time(nullptr);
    struct tm* local_time = localtime(&current_time);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", local_time);
    string time = buffer;

    // {
        
    int fd = open(filePath.c_str(), O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        return;
    }

    string temp = "\n["+time+"]["+type+"]"+content;

    
    int bytesWritten = write(fd, temp.c_str(), temp.size());
    if (bytesWritten == -1) {
        close(fd);
        return;
    }
    // }
    
    close(fd);
}

void seederLog(string type, string content){
    lock_guard <mutex> guard(seederLoggerMutex);
    
    string logDirPath = "./logs_"+seederIpPort.first+":"+to_string(seederIpPort.second);
    string filePath = logDirPath + "/seederLogs.txt";

    time_t current_time = time(nullptr);
    struct tm* local_time = localtime(&current_time);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", local_time);
    string time = buffer;

    // {
        
    
    int fd = open(filePath.c_str(), O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        return;
    }

    string temp = "\n["+time+"]["+type+"]"+content;

    int bytesWritten = write(fd, temp.c_str(), temp.size());
    if (bytesWritten == -1) {
        close(fd);
        return;
    }
    close(fd);
    // }
}

vector<string> findSHA(string filePath){

    int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd < 0) {
        raiseWarning("CustomWarning: utils.cpp line:56 filePath: " + filePath);
        raiseError("SeederError: Opening the file at findSHA", errno);
        return {};
    }
    SHA256_CTX sha256_1;
    SHA256_Init(&sha256_1);

    vector<SHA256_CTX> temp;
    char buffer[PIECE_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        SHA256_CTX sha256_2;
        SHA256_Init(&sha256_2);
        SHA256_Update(&sha256_1, buffer, bytesRead);
        SHA256_Update(&sha256_2, buffer, bytesRead);

        temp.push_back(sha256_2);
    }
    close(fileFd);

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_1);

    char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex_hash + 2 * i, "%02x", hash[i]);
    }

    vector<string> fileSHAs;
    fileSHAs.push_back(hex_hash);
    
    for(auto &it : temp) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &it);

        char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(hex_hash + 2 * i, "%02x", hash[i]);
        }
        fileSHAs.push_back(hex_hash);
    }
    return fileSHAs;
}

string findPieceSHA(string pieceData){

    SHA256_CTX sha256_2;
    SHA256_Init(&sha256_2);

    SHA256_Update(&sha256_2, pieceData.c_str(), pieceData.size());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_2);

    char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex_hash + 2 * i, "%02x", hash[i]);
    }

    return hex_hash;
}

int giveFileSize(string filePath){

    int fileFd = open(filePath.c_str(), O_RDONLY);

    if (fileFd < 0) {
        raiseError("SeederError: Opening the file at giveFileSize", errno);
        return -1;
    }

    off_t fileSize = lseek(fileFd, 0, SEEK_END);

    if (fileSize < 0) {
        cerr << "Error: seeking to end of file " << strerror(errno) << endl;
        close(fileFd);
        return -1;
    }

    return fileSize;
}

void raiseError(string errorMessage){
    cerr << string(RED) + errorMessage + "!!\n" + string(RESET) << flush;
}

void raiseError(string errorMessage, int errCode){
    cerr << string(RED) + errorMessage + "!!\n" + string(RESET) << flush;
    cerr << "ERROR : " + to_string(errno) + " - " + strerror(errCode) + "\n" << flush;
}

void raiseWarning(string warningMessage){
    cout << string(YELLOW) + warningMessage + "!!\n" + string(RESET) << flush;
}

void raiseSuccess(string successMessage){
    cout << string(GREEN) + successMessage + "!!\n" + string(RESET) << flush;
}