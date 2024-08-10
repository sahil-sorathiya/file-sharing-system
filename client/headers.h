#include <bits/stdc++.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h> // for O_RDONLY
#include <errno.h>
#include <mutex>
#include <openssl/hmac.h>
#include <openssl/sha.h>

using namespace std;

vector <string> tokenize(string buffer, char separator);
pair<pair <string, int>, pair <string, int> > processArgs(int argc, char *argv[]);