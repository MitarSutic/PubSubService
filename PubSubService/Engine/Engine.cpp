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

//void subscribe(const char* topic, int subscriber_id) {
//    // dodavanje nove pretplate
//    if (subscription_count < max_topics) {
//        subscriptions[subscription_count].topic = strdup(topic);
//        subscriptions[subscription_count].subscriber_id = subscriber_id;
//        subscription_count++;
//        printf("subscriber %d successfully subscribed to topic '%s'.\n", subscriber_id, topic);
//    }
//    else {
//        printf("error: maximum number of subscriptions reached.\n");
//    }
//}
//
//// funkcija za prikaz pretplata (za testiranje)
//void printsubscriptions() {
//    printf("current subscriptions:\n");
//    for (int i = 0; i < subscription_count; i++) {
//        printf("subscriber %d -> topic '%s'\n", subscriptions[i].subscriber_id, subscriptions[i].topic);
//    }
//}

DWORD WINAPI ClientHandler(LPVOID clientSocket) {
    SOCKET socket = (SOCKET)clientSocket;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    // Receive data from the client
    do {
        iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Null-terminate the received string
            printf("Message received from client: %s\n", recvbuf);

            // Parse the received message into Kime, Topic, and Info
            char kime[DEFAULT_BUFLEN], topic[DEFAULT_BUFLEN], info[DEFAULT_BUFLEN];

            // Try parsing the input string using sscanf_s or strtok
            if (sscanf_s(recvbuf, "Kime: %[^,], Topic: %[^,], Info: %s", kime, (unsigned)_countof(kime), topic, (unsigned)_countof(topic), info, (unsigned)_countof(info)) == 3) {
                printf("Parsed message -> Kime: %s, Topic: %s, Info: %s\n", kime, topic, info);

                // Create a PubSub object with the parsed data
                PubSub newPubSub;
                strcpy_s(newPubSub.topic, sizeof(newPubSub.topic), topic);
                strcpy_s(newPubSub.info, sizeof(newPubSub.info), info);
                newPubSub.accepted_socket = (int)socket; // You can use socket ID or another identifier

                // Insert the PubSub object into the hashmap
                if (insertPubSub(&map, kime, newPubSub)) {
                    printf("PubSub object inserted into hashmap.\n");
                    char response[DEFAULT_BUFLEN];
                    snprintf(response, sizeof(response), "Approval: Data added successfully! Kime: %s, Topic: %s, Info: %s",
                        kime, newPubSub.topic, newPubSub.info);

                    send(newPubSub.accepted_socket, response, (int)strlen(response), 0);
                    printf("Sent approval message with PubSub data to client.\n");
                }
                else {
                    printf("Failed to insert PubSub object into hashmap.\n");
                }
            }
            else {
                printf("Failed to parse message correctly.\n");
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


