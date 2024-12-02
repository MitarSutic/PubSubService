#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HashTable.h"

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

bool InitializeWindowsSockets();
DWORD WINAPI ClientHandler(LPVOID clientSocket);

// Global HashMap to store received data
HashMap map;

int main(void)
{
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    int iResult;

    // Initialize the HashMap
    initialize_hashmapClient(&map);
    // Initialize Winsock
    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    // Set up the address structure
    addrinfo* result = NULL, hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     // Allow binding

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a socket for listening
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Bind the socket
    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    // Start listening
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    printf("Server is listening on port %s.\n", DEFAULT_PORT);

    // Main loop to accept clients
    while (1)
    {
        printf("Waiting for a client...\n");
        clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            printf("accept failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }

        printf("Client connected.\n");

        // Create a thread to handle the client
        HANDLE clientThread = CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket, 0, NULL);
        if (clientThread == NULL)
        {
            printf("CreateThread failed with error: %d\n", GetLastError());
            closesocket(clientSocket);
        }
        else
        {
            CloseHandle(clientThread);
        }
    }

    // Cleanup
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}

DWORD WINAPI ClientHandler(LPVOID clientSocket) {
    SOCKET socket = (SOCKET)clientSocket;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    // Receive data from the client
    do {
        iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0); // Leave space for null-terminator
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Null-terminate the received string
            printf("Message received from client: %s\n", recvbuf);

            // Parse the received message into Option, Kime, Topic, and Info
            char option[DEFAULT_BUFLEN], kime[DEFAULT_BUFLEN], topic[DEFAULT_BUFLEN], info[DEFAULT_BUFLEN];
            memset(option, 0, sizeof(option));  // Clear buffers
            memset(kime, 0, sizeof(kime));
            memset(topic, 0, sizeof(topic));
            memset(info, 0, sizeof(info));
            char* context = NULL;  // For maintaining state in strtok_s
            char* token = NULL;

            // Tokenize the received message
            token = strtok_s(recvbuf, "-", &context); // Split on `-`
            while (token != NULL) {
                // Remove leading spaces
                while (*token == ' ') token++; // Trim leading spaces

                // Parse each part based on its prefix
                if (strncmp(token, "Option:", 7) == 0) {
                    strncpy_s(option, sizeof(option), token + 7, _TRUNCATE); // Copy after "Option:"
                }
                else if (strncmp(token, "Kime:", 5) == 0) {
                    strncpy_s(kime, sizeof(kime), token + 5, _TRUNCATE); // Copy after "Kime:"
                }
                else if (strncmp(token, "Topic:", 6) == 0) {
                    strncpy_s(topic, sizeof(topic), token + 6, _TRUNCATE); // Copy after "Topic:"
                }
                else if (strncmp(token, "Info:", 5) == 0) {
                    strncpy_s(info, sizeof(info), token + 5, _TRUNCATE); // Copy after "Info:"
                }
                else {
                    printf("Unexpected token: %s\n", token); // Debug unexpected tokens
                }

                // Move to the next token
                token = strtok_s(NULL, ",", &context);
            }

            // Validate if all required fields were parsed
            if (strlen(option) > 0 && strlen(kime) > 0 && strlen(topic) > 0 && strlen(info) > 0) {
                printf("Parsed message -> Option: %s, Kime: %s, Topic: %s, Info: %s\n", option, kime, topic, info);
            }
            else {
                printf("Failed to parse message correctly. Ensure the format is: Option: value - Kime: value, Topic: value, Info: value\n");
            }
        }
        else if (iResult == 0) {
            printf("Connection closed by client.\n");
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }
    } while (iResult > 0);

    // Cleanup
    closesocket(socket);
    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}


