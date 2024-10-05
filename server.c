#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>  // for directory functions
#include <limits.h>  // for PATH_MAX
#include "server_structs.h"


#define PORT 9090
#define BUFSIZE 512

void fatal_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Structure to hold client details
typedef struct ClientInfo {
    int clientSock;
    struct sockaddr_in clientAddr;
} ClientInfo;

int list(int clientSock, char *path) {


    return -1;
}

int diff(int clientSock, char *path) {


    return -1;
}

int pull(int clientSock, char *path) {


    return -1;
}

void handle_client(void *clientData, char *path) {
    ClientInfo *client = (ClientInfo *)clientData;
    int clientSock = client->clientSock;
    char clientIP[INET_ADDRSTRLEN];

    // Convert client IP to a string
    inet_ntop(AF_INET, &(client->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

    printf("Client connected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));

    // Echo received data back to the client
    char buffer[BUFSIZE];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSock, buffer, BUFSIZE, 0)) > 0) {
            // send(clientSock, buffer, bytesRead, 0);  // Echo back the message

            // Handle Functions: LIST, DIFF, PULL, LEAVE
            if (strcasecmp(buffer, "LIST") == 0 || strcasecmp(buffer, "1") == 0) {
                printf("Client %s:%d requested LIST\n", clientIP, ntohs(client->clientAddr.sin_port));
                if (list(clientSock, path) == -1) {
                    send(clientSock, "Failed to list files", 20, 0);
                } else {
                    send(clientSock, "LIST SUCCESSFUL", 15, 0);
                }
            } else if (strcasecmp(buffer, "DIFF") == 0 || strcasecmp(buffer, "2") == 0) {
                printf("Client %s:%d requested DIFF\n", clientIP, ntohs(client->clientAddr.sin_port));
                if (diff(clientSock, path) == -1) {
                    send(clientSock, "Failed to get differences", 27, 0);
                } else {
                    send(clientSock, "DIFF SUCCESSFUL", 16, 0);
                }
            } else if (strcasecmp(buffer, "PULL") == 0 || strcasecmp(buffer, "3") == 0) {
                printf("Client %s:%d requested PULL\n", clientIP, ntohs(client->clientAddr.sin_port));
                if (pull(clientSock, path) == -1) {
                    send(clientSock, "Failed to pull files" , 19, 0);
                } else {
                    send(clientSock, "PULL SUCCESSFUL", 17, 0);
                }
            } else if (strcasecmp(buffer, "LEAVE") == 0 || strcasecmp(buffer, "4") == 0) {
                // Disconnect client
                printf("Client %s:%d requested LEAVE\n", clientIP, ntohs(client->clientAddr.sin_port));
                send(clientSock, "Disconnecting Client...", 24, 0);
                printf("Client disconnected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));
                close(clientSock); 
                free(client);
                break;
            } else {
                // Invalid command
                send(clientSock, "Invalid command.", 17, 0);
            }
    }

    // Client disconnected    
    // printf("Client disconnected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));
    // close(clientSock); 
    // free(client);

} 

int main(int argc, char *argv[]) {
    char *path;
    path = argv[1];

    int serverSock, clientSock;
    struct sockaddr_in servAddr, clientAddr;
    pthread_t threadID;
    unsigned int clientLen;

    // Create socket
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fatal_error("Socket failed");
    }

    // Prepare server address
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(serverSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        fatal_error("Bind failed");
    }

    // Listen for incoming connections
    if (listen(serverSock, 5) < 0) {
        fatal_error("Listen failed");
    }

    printf("Server listening on port %d\n", PORT);

    // Main loop: accept client connections and handle them
    while (1) {
        clientLen = sizeof(clientAddr);
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0) {
            fatal_error("Accept failed");
        }

        // Allocate memory for client information
        ClientInfo *clientInfo = (ClientInfo *)malloc(sizeof(ClientInfo));
        clientInfo->clientSock = clientSock;
        clientInfo->clientAddr = clientAddr;

        // Create a thread to handle the client
        if (pthread_create(&threadID, NULL, (void *)handle_client, (void *)clientInfo) != 0) {
            fatal_error("Thread creation failed");
        }

        pthread_detach(threadID);  // Detach thread to handle cleanup automatically
    }

    close(serverSock);
    return 0;
}
