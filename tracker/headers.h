#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for O_RDONLY
#include <errno.h>


using namespace std;

// utils.cpp
vector <string> tokenize(string buffer, char separator);
pair <string, int> processArgs(int argc, char *argv[]);

// handlers.cpp
void handleClientRequest(int clientSocket, string clientIP, int clientPort);