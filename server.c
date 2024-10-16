#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <dirent.h>    // for directory functions
#include <sys/types.h> // for struct stat
#include <sys/stat.h>  // for struct stat
#include <limits.h>    // for PATH_MAX
#include <openssl/sha.h>
#include "server_structs.h"

#define PORT 9090
#define BUFSIZE 512

void fatal_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

// Structure to hold client details
struct FileInfo
{
    char name[256]; // File name
    char *content;  // File content
};

int compare_file_info(const void *a, const void *b)
{
    const struct FileInfo *fileInfoA = *(const struct FileInfo **)a;
    const struct FileInfo *fileInfoB = *(const struct FileInfo **)b;
    return strcmp(fileInfoA->name, fileInfoB->name);
}

// Structure to hold client details
typedef struct ClientInfo
{
    int clientSock;
    struct sockaddr_in clientAddr;
} ClientInfo;

int list(int clientSock, char *path, struct FileInfo *files[], int list)
{
    struct dirent *entry;
    struct stat fileStat; // Define a struct stat object to hold file information
    char file_path[1024];
    char buffer[1024];
    int fileCount = 0;

    // Open the directory
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Could not open directory");
        return -1;
    }

    // Read each file in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        struct FileInfo *fileInfo;
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        // Use stat() to check if the entry is a regular file
        if (stat(file_path, &fileStat) == 0 && S_ISREG(fileStat.st_mode))
        {
            // Allocate memory for file information
            fileInfo = (struct FileInfo *)malloc(sizeof(struct FileInfo));
            if (fileInfo == NULL)
            {
                perror("Memory allocation failed");
                continue;
            }

            // Copy file name into the struct
            strncpy(fileInfo->name, entry->d_name, sizeof(fileInfo->name) - 1);
            fileInfo->name[sizeof(fileInfo->name) - 1] = '\0'; // Ensure null-termination

            // Open and read the file content
            FILE *file = fopen(file_path, "r");
            if (file == NULL)
            {
                perror("Error opening file");
                free(fileInfo);
                continue; // Move to the next file
            }

            // Get the file size and read the content
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            if (fileSize < 0)
            { // Check if ftell() failed
                perror("Error determining file size");
                fclose(file);
                free(fileInfo);
                continue;
            }
            rewind(file);

            fileInfo->content = (char *)malloc(fileSize + 1);
            if (fileInfo->content == NULL)
            {
                perror("Memory allocation for file content failed");
                fclose(file);
                free(fileInfo);
                continue;
            }

            // Read the file content
            size_t bytesRead = fread(fileInfo->content, 1, fileSize, file);
            if (bytesRead < fileSize)
            {
                perror("Error reading file content");
                free(fileInfo->content);
                fclose(file);
                free(fileInfo);
                continue;
            }

            fileInfo->content[fileSize] = '\0'; // Null-terminate the content
            fclose(file);

            files[fileCount++] = fileInfo;
        }
    }

    // Close the directory
    closedir(dir);

    qsort(files, fileCount, sizeof(struct FileInfo *), compare_file_info);
    // for (int i = 0; i < fileCount; i++)
    // {
    //     printf("%s\n", files[i]->name);
    // }
    return fileCount;
}

int diff(int clientSock, char *path)
{

    return -1;
}

int pull(int clientSock, char *path)
{

    return -1;
}

void handle_client(void *clientData, char *path)
{
    ClientInfo *client = (ClientInfo *)clientData;
    int clientSock = client->clientSock;
    char clientIP[INET_ADDRSTRLEN];

    // Convert client IP to a string
    inet_ntop(AF_INET, &(client->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

    printf("Client connected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));

    // Echo received data back to the client
    char buffer[BUFSIZE];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSock, buffer, BUFSIZE, 0)) > 0)
    {
        // Handle Functions: LIST, DIFF, PULL, LEAVE
        if (strcasecmp(buffer, "LIST") == 0 || strcasecmp(buffer, "1") == 0)
        {
            printf("Client %s:%d requested LIST\n", clientIP, ntohs(client->clientAddr.sin_port));
            add_to_csv_entry("./client_db.csv", clientIP, ntohs(client->clientAddr.sin_port), "LIST");
            struct FileInfo *files[500];
            int serverFileCount = -1;
            if ((serverFileCount = list(clientSock, ".", files, 1)) == -1)
            {
                send(clientSock, "Failed to list files", 20, 0);
            }
            else
            {
                send(clientSock, &serverFileCount, sizeof(int), 0);
                for (int i = 0; i < serverFileCount; i++)
                {
                    int nameLen = strlen(files[i]->name);
                    send(clientSock, &nameLen, sizeof(int), 0);
                    send(clientSock, files[i]->name, nameLen, 0);
                }

                send(clientSock, "LIST SUCCESSFUL\n", 16, 0);
            }
        }
        else if (strcasecmp(buffer, "DIFF") == 0 || strcasecmp(buffer, "2") == 0)
        {
            printf("Client %s:%d requested DIFF\n", clientIP, ntohs(client->clientAddr.sin_port));
            struct FileInfo *files[500];
            int serverFileCount = -1;
            if ((serverFileCount = list(clientSock, ".", files, 0)) == -1)
            {
                send(clientSock, "Failed to get differences", 27, 0);
            }
            else
            {
                int diff = 0;
                for (int i = 0; i < serverFileCount; i++)
                {
                    if (strcasecmp(files[i]->name, "server") == 0 || strcasecmp(files[i]->name, "client_db.csv") == 0)
                    {
                        serverFileCount--;
                        diff++;
                    }
                }
                send(clientSock, &serverFileCount, sizeof(int), 0);
                for (int i = 0; i < serverFileCount + diff; i++)
                {
                    // Ignore ./server and client_db.csv
                    if (strcasecmp(files[i]->name, "server") == 0 || strcasecmp(files[i]->name, "client_db.csv") == 0)
                        continue;

                    SHA256_CTX sha256;
                    SHA256_Init(&sha256);
                    SHA256_Update(&sha256, files[i]->content, strlen(files[i]->content));

                    int nameLen = strlen(files[i]->name);
                    send(clientSock, &nameLen, sizeof(int), 0);
                    send(clientSock, files[i]->name, nameLen, 0);

                    unsigned char hash[SHA256_DIGEST_LENGTH];
                    SHA256_Final(hash, &sha256);

                    send(clientSock, hash, SHA256_DIGEST_LENGTH, 0);
                }

                send(clientSock, "DIFF SUCCESSFUL\n", 17, 0);
            }
            add_to_csv_entry("./client_db.csv", clientIP, ntohs(client->clientAddr.sin_port), "DIFF");
        }
        else if (strcasecmp(buffer, "PULL") == 0 || strcasecmp(buffer, "3") == 0)
        {
            printf("Client %s:%d requested PULL\n", clientIP, ntohs(client->clientAddr.sin_port));
            struct FileInfo *files[500];
            int serverFileCount = -1;
            if ((serverFileCount = list(clientSock, ".", files, 0)) == -1)
            {
                send(clientSock, "Failed to get differences", 27, 0);
            }
            else
            {
                // Retrieve Client Hashes
                int clientFileCount;
                recv(clientSock, &clientFileCount, sizeof(int), 0); // Receive the number of files
                unsigned char clientHashes[clientFileCount][SHA256_DIGEST_LENGTH];
                unsigned char clientFileNames[500][256];
                // printf("Client FC: %d\n", clientFileCount);

                for (int i = 0; i < clientFileCount; i++)
                {
                    int nameLen;
                    recv(clientSock, &nameLen, sizeof(int), 0); // Receive the length of the file name

                    char fileName[256];
                    recv(clientSock, fileName, nameLen, 0); // Receive the file name
                    fileName[nameLen] = '\0';

                    for (int j = 0; j < nameLen; j++)
                    {
                        clientFileNames[i][j] = fileName[j];
                    }
                    clientFileNames[i][nameLen] = '\0';
                    // for (int j = 0; j < nameLen; j++) {
                    //     printf("%c", clientFileNames[i][j]);
                    // }
                    // printf("\n");

                    recv(clientSock, clientHashes[i], SHA256_DIGEST_LENGTH, 0); // Receive the file hash
                }

                // Get server hashes
                unsigned char serverHashes[serverFileCount][SHA256_DIGEST_LENGTH];
                for (int i = 0; i < serverFileCount; i++)
                {
                    // Ignore ./server and client_db.csv
                    if (strcasecmp(files[i]->name, "server") == 0 || strcasecmp(files[i]->name, "client_db.csv") == 0)
                        continue;

                    SHA256_CTX sha256;
                    SHA256_Init(&sha256);
                    SHA256_Update(&sha256, files[i]->content, strlen(files[i]->content));
                    SHA256_Final(serverHashes[i], &sha256);
                }

                // printf("ServerFileCount: %d\n", serverFileCount);
                // printf("ClientFileCount: %d\n", clientFileCount);

                // Compare Hashes
                int FTS = 0;
                int filesToSend[serverFileCount];
                for (int i = 0; i < serverFileCount; i++)
                {
                    // Ignore ./server and client_db.csv
                    if (strcasecmp((const char *)files[i]->name, "server") == 0 ||
                        strcasecmp((const char *)files[i]->name, "client_db.csv") == 0)
                    {
                        // printf("Ignoring %s\n", files[i]->name);
                        continue;
                    }
                    int found = 0;
                    for (int j = 0; j < clientFileCount; j++)
                    {
                        // Debug print serverhashes and clientFileNames
                        // printf("Server File Name: %s\n", files[i]->name);
                        // printf("Server hash: ");
                        // for (int k = 0; k < SHA256_DIGEST_LENGTH; k++) {
                        //     printf("%02x", serverHashes[i][k]);
                        // }
                        // printf("\n");
                        // printf( "Client File Name: %s\n", clientFileNames[j]);
                        // printf("Client hash: ");
                        // for (int k = 0; k < SHA256_DIGEST_LENGTH; k++) {
                        //     printf("%02x", clientFileNames[j][k]);
                        // }
                        // printf("\n");


                        if (strcmp((const char *)clientFileNames[j], files[i]->name) == 0)
                        {
                            if (memcmp(serverHashes[i], clientHashes[j], SHA256_DIGEST_LENGTH) == 0)
                            {
                                found = 1;
                                break;
                            }
                        }
                    }
                    if (found == 0)
                    {
                        printf("Sending %s to client %s:%d\n", files[i]->name, clientIP, ntohs(client->clientAddr.sin_port));
                       // Store in filesToSend
                       filesToSend[FTS] = i;
                       FTS++;
                    }
                }

                // Send files back to client
                send(clientSock, &FTS, sizeof(int), 0); // Send the number of files to send
                for (int i = 0; i < FTS; i++) 
                {
                    int nameLen = strlen(files[filesToSend[i]]->name);
                    send(clientSock, &nameLen, sizeof(int), 0);
                    // Send the file name
                    send(clientSock, files[filesToSend[i]]->name, nameLen, 0);
                    // Send the file size
                    int fileSize = strlen(files[filesToSend[i]]->content);
                    send(clientSock, &fileSize, sizeof(int), 0);
                    // Send the file content
                    send(clientSock, files[filesToSend[i]]->content, fileSize, 0);
                }
                
                // Acknowlege Success
                send(clientSock, "PULL SUCCESSFUL\n", 17, 0);
            }
            add_to_csv_entry("./client_db.csv", clientIP, ntohs(client->clientAddr.sin_port), "PULL");
        }
        else if (strcasecmp(buffer, "LEAVE") == 0 || strcasecmp(buffer, "4") == 0 || strcasecmp(buffer, "EXIT") == 0)
        {
            // Disconnect client
            printf("Client %s:%d requested LEAVE\n", clientIP, ntohs(client->clientAddr.sin_port));
            add_to_csv_entry("./client_db.csv", clientIP, ntohs(client->clientAddr.sin_port), "LEAVE");
            break;
        }
        else
        {
            // Invalid command
            // add_to_csv_entry("./client_db.csv", clientIP, ntohs(client->clientAddr.sin_port), "INV");
            // send(clientSock, "Invalid command\n", 16, 0);
        }
        fflush(stdout);
    }

    // Client disconnected
    printf("Client disconnected: %s:%d\n", clientIP, ntohs(client->clientAddr.sin_port));
    close(clientSock);
    free(client);
}

int main(int argc, char *argv[])
{
    char *path;
    path = argv[1];

    int serverSock, clientSock;
    struct sockaddr_in servAddr, clientAddr;
    pthread_t threadID;
    unsigned int clientLen;

    // Create socket
    if ((serverSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fatal_error("Socket failed");
    }

    // Prepare server address
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(serverSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        fatal_error("Bind failed");
    }

    // Listen for incoming connections
    if (listen(serverSock, 5) < 0)
    {
        fatal_error("Listen failed");
    }

    // Create client file
    if (create_clients_csv("./client_db.csv") == -1)
    {
        fatal_error("Failed to create client database");
    }

    printf("Server listening on port %d\n", PORT);

    // Main loop: accept client connections and handle them
    while (1)
    {
        clientLen = sizeof(clientAddr);
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSock < 0)
        {
            fatal_error("Accept failed");
        }

        // Allocate memory for client information
        ClientInfo *clientInfo = (ClientInfo *)malloc(sizeof(ClientInfo));
        clientInfo->clientSock = clientSock;
        clientInfo->clientAddr = clientAddr;

        // Create a thread to handle the client
        if (pthread_create(&threadID, NULL, (void *)handle_client, (void *)clientInfo) != 0)
        {
            fatal_error("Thread creation failed");
        }

        pthread_detach(threadID); // Detach thread to handle cleanup automatically
    }

    close(serverSock);
    return 0;
}
