/*///////////////////////////////////////////////////////////
*
* FILE:	client.cpp
* PROJECT:	CNT 4007 Project 2 - Professor Traynor
* DESCRIPTION:	Client Code to interact with server
*
*////////////////////////////////////////////////////////////
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define DEFAULT_PORT "27015"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <ostream>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


int main(int argc, char *argv[]) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create socket
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    int addr = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
    if (addr != 0) {
    printf("getaddrinfo failed: %d\n", addr);
    WSACleanup();
    return 1;
    }

    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket = INVALID_SOCKET;

    // Attempt to connect to the first address returned by
    // the call to getaddrinfo
    ptr=result;

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, 
    ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
    printf("Error at socket(): %d\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
    }

    return 0;
}
