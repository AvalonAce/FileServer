#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFSIZE 512
#define PORT 9090

void fatal_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in servAddr;
    char message[BUFSIZE];
    char buffer[BUFSIZE];
    ssize_t bytesReceived;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fatal_error("Socket failed");
    }

    // Set server address
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // Connect to localhost
    servAddr.sin_port = htons(PORT);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        fatal_error("Connect failed");
    }

    printf("Connected to the server. Type a message to send:\n");

    // Send messages to the server and receive the response
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFSIZE, stdin);

        // Remove the newline character from the message
        message[strcspn(message, "\n")] = 0;

        // Send the message to the server
        send(sock, message, strlen(message), 0);

        // If the user types "exit", break the loop and disconnect
        if (strcmp(message, "exit") == 0) {
            break;
        }

        // Receive the server's response (echo)
        bytesReceived = recv(sock, buffer, BUFSIZE, 0);
        buffer[bytesReceived] = '\0';
        printf("Echo from server: %s\n", buffer);
    }

    // Close the socket
    close(sock);
    printf("Disconnected from the server.\n");

    return 0;
}
