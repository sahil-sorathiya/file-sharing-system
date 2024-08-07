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
            }
            
            string inputFromTracker = "Data Recieved!!";
            if(send(clientSocket, inputFromTracker.c_str(), inputFromTracker.size(), 0) < 0){
                cerr << "Error sending message to client " + string(strerror(errno))  + "\n" << flush;
                close(clientSocket);
                break;
            }
        }
}