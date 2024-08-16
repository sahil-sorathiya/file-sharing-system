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

    char buffer[524288];

    read(fd, buffer, sizeof(buffer));

    vector <string> ipAndPorts = tokenize(buffer, '\n');

    string trackerIp = ipAndPorts[2*(trackerNumber - 1)];
    int trackerPort = stoi(ipAndPorts[2*(trackerNumber - 1) + 1]);
    return {trackerIp, trackerPort};
}

string generateToken(string payload) {
    string secret_key = SECRET_KEY;
    // Get current time and calculate expiry time
    time_t currentTime = time(nullptr);
    time_t expiryTime = currentTime + TOKEN_EXPIRY_DURATION;
    
    string message = payload + ":" + to_string(expiryTime);

    // Generate HMAC-SHA256 signature
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), secret_key.c_str(), secret_key.length(),
                  (unsigned char*)message.c_str(), message.length(), nullptr, nullptr);

    // Convert to hexadecimal string
    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    // Return the token: payload:expiry_time:signature
    return message + ":" + string(mdString);
}

string validateToken(string token) {
    string secret_key = SECRET_KEY;
    vector <string> tokens = tokenize(token, ':');
    if(tokens.size() != 3) return "";
    
    string payload = tokens[0];
    time_t expiryTime = stol(tokens[1]);
    string signature = tokens[2];
    string payloadExpiry = payload + ":" + to_string(expiryTime);

    // Regenerate the HMAC-SHA256 signature
    unsigned char* digest;
    digest = HMAC(EVP_sha256(), secret_key.c_str(), secret_key.length(),
                  (unsigned char*)payloadExpiry.c_str(), payloadExpiry.length(), nullptr, nullptr);

    // Convert to hexadecimal string
    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    // Validate the signature
    if(signature != string(mdString)) return "";

    // Check if the token is expired
    time_t currentTime = time(nullptr);
    if (currentTime > expiryTime) return "";

    return payload;
}