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

pair <string, int> processArgs(int argc, char *argv[]){
    if(argc != 3){
        cout << "Invalid arguments!!\n" << flush;
        exit(0);
    }

    char *trackerInfoFileName = argv[1];
    int trackerNumber = atoi(argv[2]);

    int fd = open(trackerInfoFileName, O_RDONLY);
    if(fd < 0) {
        string s = trackerInfoFileName;
        cout << "Error opening " + s + " file\n" << flush;
        exit(0); 
    }

    char buffer[10240];

    read(fd, buffer, 10240);

    vector <string> ipAndPorts = tokenize(buffer, '\n');

    string trackerIp = ipAndPorts[2*(trackerNumber - 1)];
    int trackerPort = stoi(ipAndPorts[2*(trackerNumber - 1) + 1]);
    return {trackerIp, trackerPort};
}