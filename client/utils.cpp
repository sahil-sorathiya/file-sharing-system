#include "headers.h"


vector <string> tokenize(string buffer, char separator){
    vector <string> ans;
    string temp;
    for(auto it: buffer){
        if(it == separator) {
            ans.push_back(temp);
            temp.clear();
        }
        else temp.push_back(it);
    }
    if(temp.size()) ans.push_back(temp);
    return ans;
}

pair<pair <string, int>, pair <string, int> > processArgs(int argc, char *argv[]){

    if(argc != 3){
        cout << "Invalid arguments!!\n" << flush;
        exit(0);
    }
    char *trackerInfoFileName = argv[2];
    char *seederIpAndPort = argv[1];
    
    vector <string> seederIpPort = tokenize(seederIpAndPort, ':');
    string seederIp = seederIpPort[0];
    int seederPort = stoi(seederIpPort[1]);
    
    int fd = open(trackerInfoFileName, O_RDONLY);
    if(fd < 0) {
        string s = trackerInfoFileName;
        cout << "Error opening " + s + " file\n" << flush;
        exit(0); 
    }
    
    char buffer[10240];
    read(fd, buffer, 10240);

    vector <string> ipAndPorts = tokenize(buffer, '\n');
    string trackerIp = ipAndPorts[0];
    int trackerPort = stoi(ipAndPorts[1]);

    return {{seederIp, seederPort}, {trackerIp, trackerPort}};
}


vector<string> findSHA(const char* filePath) {

    int fileFd = open(filePath, O_RDONLY);

    if (fileFd < 0) {
        perror("Error: Opening the file!!");
        return {};
    }
    SHA256_CTX sha256_1, sha256_2;
    SHA256_Init(&sha256_1);

    vector<SHA256_CTX> temp;
    char buffer[PIECE_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
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

string findPieceSHA(string pieceData) {

    SHA256_CTX sha256_2;
    SHA256_Init(&sha256_2);

    SHA256_Update(&sha256_2, pieceData, pieceData.size());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256_2);

    char hex_hash[2 * SHA256_DIGEST_LENGTH + 1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex_hash + 2 * i, "%02x", hash[i]);
    }

    return hex_hash;
}

int giveFileSize(const char *filePath){

    int fileFd = open(filePath, O_RDONLY);

    if (fileFd == -1) {
        cerr << "Error: opening file " << strerror(errno) << endl;
        return -1;
    }

    off_t fileSize = lseek(fileFd, 0, SEEK_END);

    if (fileSize == -1) {
        cerr << "Error: seeking to end of file " << strerror(errno) << endl;
        close(fileFd);
        return -1;
    }

    return fileSize;
}
