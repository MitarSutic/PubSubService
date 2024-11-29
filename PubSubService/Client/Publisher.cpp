#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016" // Change to "27017" for the second client
#define SERVER_IP "127.0.0.1" // Replace with the server's IP address

#pragma comment(lib, "Ws2_32.lib")

bool InitializeWindowsSockets();

int main(void)
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    char sendbuf[DEFAULT_BUFLEN] = "Hello from the publisher!";
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    if (InitializeWindowsSockets() == false)
    {
        return 1;
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
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to the server
    connectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    iResult = connect(connectSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("Unable to connect to server!\n");
        closesocket(connectSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    printf("Connected to server. Sending message...\n");

    // Send a message to the server
    iResult = send(connectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes sent: %d\n", iResult);

    // Receive data from the server
    iResult = recv(connectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        recvbuf[iResult] = '\0'; // Null-terminate the received string
        printf("Bytes received: %d\n", iResult);
        printf("Message from server: %s\n", recvbuf);
    }
    else if (iResult == 0)
    {
        printf("Connection closed by server.\n");
    }
    else
    {
        printf("recv failed with error: %d\n", WSAGetLastError());
    }

    // Shutdown the connection
    iResult = shutdown(connectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    // Cleanup
    closesocket(connectSocket);
    WSACleanup();

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
