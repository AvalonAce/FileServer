#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>  // for struct stat
#include <sys/types.h> // for struct stat
#include <openssl/sha.h>

#define MAX_FILES 100 // Maximum number of files to handle

#define BUFSIZE 512
#define PORT 9090

void fatal_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

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

int read_files(char *path, struct FileInfo *files[])
{
    struct dirent *entry;
    struct stat fileStat; // Define a struct stat object to hold file information
    char file_path[1024];
    int fileCount = 0;

    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        perror("Could not open directory");
        return -1;
    }

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
            FILE *file = fopen(file_path, "rb"); // Open in binary mode
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

            // Store the fileInfo pointer in the files array
            files[fileCount++] = fileInfo; // Store the pointer to the FileInfo
        }
    }

    closedir(dir);

    // Sort files by name
    qsort(files, fileCount, sizeof(struct FileInfo *), compare_file_info);

    // Print all file names at the end
    // for (int i = 0; i < fileCount; i++)
    // {
    //     printf("%s\n", files[i]->name);
    // }

    return fileCount; // Return the number of files read
}

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in servAddr;
    char message[BUFSIZE];
    char buffer[BUFSIZE];
    ssize_t bytesReceived;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fatal_error("Socket failed");
    }

    // Set server address
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost
    servAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        fatal_error("Connect failed");
    }

    printf("=== Connected to the server. Welcome to UFmyMusic! ===\n");
    printf("Commands:\n1) List\n2) Diff\n3) Pull\n4) Leave\n");
    fflush(stdout);

    // Send messages to the server and receive the response
    while (1)
    {
        printf("Enter Command: ");
        fgets(message, BUFSIZE, stdin);
        fflush(stdout);

        // Remove the newline character from the message
        message[strcspn(message, "\n")] = 0;

        // If the user types "exit", break the loop and disconnect
        if (strcasecmp(message, "exit") == 0 || strcasecmp(message, "leave") == 0 || strcasecmp(message, "4") == 0)
        {
            send(sock, message, strlen(message), 0); // Send the exit message to the server
            break;
        }

        if (strcasecmp(message, "list") == 0 || strcasecmp(message, "1") == 0)
        {
            send(sock, message, strlen(message), 0);

            int fileCount;
            recv(sock, &fileCount, sizeof(int), 0); // Receive the number of files

            for (int i = 0; i < fileCount; i++)
            {

                int nameLen;
                recv(sock, &nameLen, sizeof(int), 0); // Receive the length of the file name

                char fileName[256];
                recv(sock, fileName, nameLen, 0);
                fileName[nameLen] = '\0';

                // Ignore ./server and client_db.csv
                if (strcasecmp(fileName, "server") == 0 || strcasecmp(fileName, "client_db.csv") == 0) continue;

                printf("%s\n", fileName);
            }

            bytesReceived = recv(sock, buffer, BUFSIZE, 0);
            buffer[bytesReceived] = '\0';
            printf("%s", buffer);
        }
        else if (strcasecmp(message, "diff") == 0 || strcasecmp(message, "2") == 0)
        {
            struct FileInfo *clientFiles[500];
            // struct FileInfo serverFiles[500];
            unsigned char serverFileNames[500][256];

            int clientFileCount = read_files(".", clientFiles);

            // create an array of SHA256 hashes for each file
            unsigned char clientHashes[clientFileCount][SHA256_DIGEST_LENGTH];

            for (int i = 0; i < clientFileCount; i++)
            {
                // Ignore ./server and client_db.csv
                if (strcasecmp(clientFiles[i]->name, "server") == 0 || strcasecmp(clientFiles[i]->name, "client_db.csv") == 0)
                    continue;

                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, clientFiles[i]->content, strlen(clientFiles[i]->content));
                SHA256_Final(clientHashes[i], &sha256);
            }

            send(sock, message, strlen(message), 0);

            int fileCount;
            recv(sock, &fileCount, sizeof(int), 0); // Receive the number of files

            unsigned char serverHashes[fileCount][SHA256_DIGEST_LENGTH];

            for (int i = 0; i < fileCount; i++)
            {
                int nameLen;
                recv(sock, &nameLen, sizeof(int), 0); // Receive the length of the file name

                char fileName[256];
                recv(sock, fileName, nameLen, 0);
                fileName[nameLen] = '\0';
                for (int j = 0; j < nameLen; j++)
                {
                    serverFileNames[i][j] = fileName[j];
                }
                serverFileNames[i][nameLen] = '\0';

                recv(sock, serverHashes[i], SHA256_DIGEST_LENGTH, 0);
            }

            // compare server and client files
            for (int i = 0; i < fileCount; i++)
            {
                // Ignore ./server and client_db.csv
                if (strcasecmp((const char *)serverFileNames[i], "server") == 0 ||
                    strcasecmp((const char *)serverFileNames[i], "client_db.csv") == 0)
                {
                    printf("Ignoring %s\n", serverFileNames[i]);
                    continue;
                }
                int found = 0;
                for (int j = 0; j < clientFileCount; j++)
                {
                    if (strcmp((const char *)serverFileNames[i], clientFiles[j]->name) == 0)
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
                    printf("%s is different\n", serverFileNames[i]);
                }
            }

            bytesReceived = recv(sock, buffer, BUFSIZE, 0);
            buffer[bytesReceived] = '\0';
            printf("%s", buffer);
        }
        else if (strcasecmp(message, "pull") == 0 || strcasecmp(message, "3") == 0)
        {
            struct FileInfo *clientFiles[500];
            int clientFileCount = read_files(".", clientFiles);

            // create an array of SHA256 hashes for each file
            unsigned char clientHashes[clientFileCount][SHA256_DIGEST_LENGTH];
            for (int i = 0; i < clientFileCount; i++)
            {
                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, clientFiles[i]->content, strlen(clientFiles[i]->content));
                SHA256_Final(clientHashes[i], &sha256);
            }

            send(sock, message, strlen(message), 0); // Send PULL
            // printf("Pull sent\n");
            // Send Hashes to be compared at server ---
            
            // Send Hashes of client
            send(sock, &clientFileCount, sizeof(int), 0);
            for (int i = 0; i < clientFileCount; i++)
            {
                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, clientFiles[i]->content, strlen(clientFiles[i]->content));

                int nameLen = strlen(clientFiles[i]->name);
                send(sock, &nameLen, sizeof(int), 0); // Send Name Length
                send(sock, clientFiles[i]->name, nameLen, 0); // Send Name

                unsigned char hash[SHA256_DIGEST_LENGTH];
                SHA256_Final(hash, &sha256);

                send(sock, hash, SHA256_DIGEST_LENGTH, 0); // Send Hash
            }

            // printf("Hashes sent\n");
            // Receive Files from Server
            int filesFromServ = 0;
            recv(sock, &filesFromServ, sizeof(int), 0);
            FILE *fp;
            for (int i = 0; i < filesFromServ; i++)
            {
                // Get Name
                   int nameLen;
                   recv(sock, &nameLen, sizeof(int), 0);
                   char fileName[256] = {0};
                   recv(sock, fileName, nameLen, 0);
                   fileName[nameLen] = '\0';

                   // Get Content
                   int contentLen;
                   recv(sock, &contentLen, sizeof(int), 0);
                   char content[contentLen];
                   recv(sock, content, contentLen, 0); // Receive Content

                    // Write
                    fp = fopen(fileName, "w+");
                   if (fp == NULL) {
                     perror("Error opening file for writing");
                     exit(1);
                   }
                   fwrite(content, 1, contentLen, fp);
                   printf("Successfully pulled %s\n", fileName);
                    fclose(fp);
            }
            


           bytesReceived = recv(sock, buffer, BUFSIZE, 0);
            buffer[bytesReceived] = '\0';
            printf("%s", buffer);
        }
        else
        {
            printf("Invalid command\n");
        }

        fflush(stdout);
    }

    // Close the socket
    close(sock);
    printf("=== Disconnected from the server ===\n");

    return 0;
}
