#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016" // Server port
#define SERVER_IP "127.0.0.1" // Replace with the server's IP address

#pragma comment(lib, "Ws2_32.lib")

SOCKET connectSocket = INVALID_SOCKET;
int iResult;
char sendbuf[DEFAULT_BUFLEN];
char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

bool InitializeWindowsSockets();
void Connect();
void Connection();

int main(void) {
    char option[12];

    printf("Type 1 to connect: ");
    scanf_s("%s", option, sizeof(option));
    getchar(); // Clear newline left in the buffer

    if (strcmp("1", option) == 0) {
        Connect();
    }
    else {
        printf("Invalid option. Exiting.\n");
        return 1;
    }

    // Enter the communication loop
    Connection();

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();
    printf("Connection closed. Goodbye!\n");
    return 0;
}

bool InitializeWindowsSockets() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
    return true;
}

void Connect() {
    // Initialize Winsock
    if (InitializeWindowsSockets() == false) {
        printf("Initialize error!\n");
        exit(EXIT_FAILURE);
    }

    // Set up the address structure for the server
    addrinfo* result = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol

    // Resolve the server address and port
    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Create a SOCKET for connecting to the server
    connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connectSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    iResult = connect(connectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("Unable to connect to server!\n");
        closesocket(connectSocket);
        freeaddrinfo(result);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);
    printf("Connected to server.\n");
}

void Connection() {
    while (1) {
        char kime[128] = { 0 };
        char topic[128] = { 0 };
        char info[128] = { 0 };
        char pubsub[12] = { 0 };

        printf("\nChoose an action (publish, subscribe, disconnect): ");
        scanf_s("%s", pubsub, sizeof(pubsub));

        if (strcmp(pubsub, "disconnect") == 0) {
            printf("Disconnecting from server...\n");
            break;
        }

        if (strcmp(pubsub, "Publish") == 0) {
            getchar();
            printf("Enter kime: ");
            fgets(kime, sizeof(kime), stdin);
            kime[strcspn(kime, "\n")] = '\0';

            printf("Enter topic: ");
            fgets(topic, sizeof(topic), stdin);
            topic[strcspn(topic, "\n")] = '\0';

            printf("Enter info: ");
            fgets(info, sizeof(info), stdin);
            info[strcspn(info, "\n")] = '\0';

            // Format the message
            snprintf(sendbuf, sizeof(sendbuf), "Option: Publish - Kime: %s, Topic: %s, Info: %s",kime, topic, info);
        }
        else if (strcmp(pubsub, "subscribe") == 0) {
            printf("Enter topic to subscribe: ");
            fgets(topic, sizeof(topic), stdin);
            topic[strcspn(topic, "\n")] = '\0';
            snprintf(sendbuf, sizeof(sendbuf), "Subscribe - Topic: %s", topic);
        }
        else {
            printf("Invalid action. Try again.\n");
            continue;
        }

        // Send the message to the server
        iResult = send(connectSocket, sendbuf, (int)strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            break;
        }

        printf("Message sent: %s\n", sendbuf);

        // Receive a response from the server
        iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0'; // Null-terminate the received string
            printf("Server: %s\n", recvbuf);
        }
        else if (iResult == 0) {
            printf("Connection closed by server.\n");
            break;
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }
    }
}
