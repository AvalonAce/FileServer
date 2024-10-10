#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>  // for directory functions
#include <sys/types.h>  // for struct stat
#include <sys/stat.h>  // for struct stat
#include <limits.h>  // for PATH_MAX
#include "server_structs.h"


#define PORT 9090
#define BUFSIZE 512

void fatal_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Structure to hold client details
struct FileInfo {
    char name[256];  // File name
    char *content;   // File content
};

// Structure to hold client details
typedef struct ClientInfo {
    int clientSock;
    struct sockaddr_in clientAddr;
} ClientInfo;

int list(int clientSock, char *path, struct FileInfo **files, int *fileCount) {
    struct dirent *entry;
    struct stat fileStat;   // Define a struct stat object to hold file information
    struct FileInfo *fileInfo;
    char file_path[1024];
    char buffer[1024];

    // Open the directory
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    // Read each file in the directory
    while ((entry = readdir(dir)) != NULL) {
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        // Use stat() to check if the entry is a regular file
        if (stat(file_path, &fileStat) == 0 && S_ISREG(fileStat.st_mode)) {
            // Allocate memory for file information
            fileInfo = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (fileInfo == NULL) {
                perror("Memory allocation failed");
                continue;
            }

            // Copy file name into the struct
            strncpy(fileInfo->name, entry->d_name, sizeof(fileInfo->name) - 1);
            fileInfo->name[sizeof(fileInfo->name) - 1] = '\0';  // Ensure null-termination

            // Open and read the file content
            FILE *file = fopen(file_path, "r");
            if (file == NULL) {
                perror("Error opening file");
                free(fileInfo);
                continue;  // Move to the next file
            }

            // Get the file size and read the content
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            if (fileSize < 0) { // Check if ftell() failed
                perror("Error determining file size");
                fclose(file);
                free(fileInfo);
                continue;
            }
            rewind(file);

            fileInfo->content = (char *)malloc(fileSize + 1);
            if (fileInfo->content == NULL) {
                perror("Memory allocation for file content failed");
                fclose(file);
                free(fileInfo);
                continue;
            }

            // Read the file content
            size_t bytesRead = fread(fileInfo->content, 1, fileSize, file);
            if (bytesRead < fileSize) {
                perror("Error reading file content");
                free(fileInfo->content);
                fclose(file);
                free(fileInfo);
                continue;
            }

            fileInfo->content[fileSize] = '\0';  // Null-terminate the content
            fclose(file);

            // Send file information to the client (or perform any other action)
            snprintf(buffer, sizeof(buffer), "%s\n", fileInfo->name);
            if (send(clientSock, buffer, strlen(buffer), 0) < 0) {
                perror("Error sending file information to the client");
            }

            // Free allocated memory
            free(fileInfo->content);
            free(fileInfo);
        }
    }

    // Close the directory
    closedir(dir);

    return 0;
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
            // Handle Functions: LIST, DIFF, PULL, LEAVE
            if (strcasecmp(buffer, "LIST") == 0 || strcasecmp(buffer, "1") == 0) {
                printf("Client %s:%d requested LIST\n", clientIP, ntohs(client->clientAddr.sin_port));
                add_to_csv_entry("./cilent_database.csv", clientIP, ntohs(client->clientAddr.sin_port), "LIST");
                struct FileInfo *files;
                int fileCount;
                if (list(clientSock, ".", &files, &fileCount) == -1) {
                    send(clientSock, "Failed to list files", 20, 0);
                } else {
                    send(clientSock, "LIST SUCCESSFUL\n", 16, 0);
                }
            } else if (strcasecmp(buffer, "DIFF") == 0 || strcasecmp(buffer, "2") == 0) {
                printf("Client %s:%d requested DIFF\n", clientIP, ntohs(client->clientAddr.sin_port));
                add_to_csv_entry("./cilent_database.csv", clientIP, ntohs(client->clientAddr.sin_port), "DIFF");
                if (diff(clientSock, path) == -1) {
                    send(clientSock, "Failed to get differences", 27, 0);
                } else {
                    send(clientSock, "DIFF SUCCESSFUL", 16, 0);
                }
            } else if (strcasecmp(buffer, "PULL") == 0 || strcasecmp(buffer, "3") == 0) {
                printf("Client %s:%d requested PULL\n", clientIP, ntohs(client->clientAddr.sin_port));
                add_to_csv_entry("./cilent_database.csv", clientIP, ntohs(client->clientAddr.sin_port), "PULL");
                if (pull(clientSock, path) == -1) {
                    send(clientSock, "Failed to pull files" , 19, 0);
                } else {
                    send(clientSock, "PULL SUCCESSFUL", 17, 0);
                }
            } else if (strcasecmp(buffer, "LEAVE") == 0 || strcasecmp(buffer, "4") == 0) {
                // Disconnect client
                printf("Client %s:%d requested LEAVE\n", clientIP, ntohs(client->clientAddr.sin_port));
                add_to_csv_entry("./cilent_database.csv", clientIP, ntohs(client->clientAddr.sin_port), "LEAVE");
                send(clientSock, "Disconnecting Client...", 24, 0);
                printf("Client disconnected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));
                close(clientSock); 
                free(client);
                break;
            } else {
                // Invalid command
                add_to_csv_entry("./cilent_database.csv", clientIP, ntohs(client->clientAddr.sin_port), "INV");
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

    // Create client file
    if (create_clients_csv("./cilent_database.csv") == -1) {
        fatal_error("Failed to create client database");
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
