#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define PORT1 "27016" // Port for first client
#define PORT2 "27017" // Port for second client

bool InitializeWindowsSockets();
DWORD WINAPI ClientHandler(LPVOID clientSocket);

int main(void)
{
    SOCKET listenSocket1 = INVALID_SOCKET;
    SOCKET listenSocket2 = INVALID_SOCKET;
    SOCKET clientSocket1 = INVALID_SOCKET;
    SOCKET clientSocket2 = INVALID_SOCKET;
    int iResult;

    if (InitializeWindowsSockets() == false)
    {
        return 1;
    }

    // Prepare address information structures for both sockets
    addrinfo* result1 = NULL, * result2 = NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4 address
    hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
    hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
    hints.ai_flags = AI_PASSIVE;     //

    // Resolve the server address and port for first socket
    iResult = getaddrinfo(NULL, PORT1, &hints, &result1);
    if (iResult != 0)
    {
        printf("getaddrinfo failed for PORT1 with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Resolve the server address and port for second socket
    iResult = getaddrinfo(NULL, PORT2, &hints, &result2);
    if (iResult != 0)
    {
        printf("getaddrinfo failed for PORT2 with error: %d\n", iResult);
        freeaddrinfo(result1);
        WSACleanup();
        return 1;
    }

    // Create listening sockets
    listenSocket1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket1 == INVALID_SOCKET)
    {
        printf("socket failed for PORT1 with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result1);
        freeaddrinfo(result2);
        WSACleanup();
        return 1;
    }

    listenSocket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket2 == INVALID_SOCKET)
    {
        printf("socket failed for PORT2 with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result1);
        freeaddrinfo(result2);
        closesocket(listenSocket1);
        WSACleanup();
        return 1;
    }

    // Bind listening sockets to their respective ports
    iResult = bind(listenSocket1, result1->ai_addr, (int)result1->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed for PORT1 with error: %d\n", WSAGetLastError());
        freeaddrinfo(result1);
        freeaddrinfo(result2);
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }

    iResult = bind(listenSocket2, result2->ai_addr, (int)result2->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed for PORT2 with error: %d\n", WSAGetLastError());
        freeaddrinfo(result1);
        freeaddrinfo(result2);
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }

    // Free address info
    freeaddrinfo(result1);
    freeaddrinfo(result2);

    // Put listening sockets in listening mode
    iResult = listen(listenSocket1, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed for PORT1 with error: %d\n", WSAGetLastError());
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }

    iResult = listen(listenSocket2, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed for PORT2 with error: %d\n", WSAGetLastError());
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }

    printf("Server initialized, waiting for clients on PORT1 and PORT2.\n");

    // Accept clients on two separate threads
    printf("Waiting for Client 1 on PORT1...\n");
    clientSocket1 = accept(listenSocket1, NULL, NULL);
    if (clientSocket1 == INVALID_SOCKET)
    {
        printf("accept failed for PORT1 with error: %d\n", WSAGetLastError());
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        WSACleanup();
        return 1;
    }
    printf("Client 1 connected on PORT1.\n");

    printf("Waiting for Client 2 on PORT2...\n");
    clientSocket2 = accept(listenSocket2, NULL, NULL);
    if (clientSocket2 == INVALID_SOCKET)
    {
        printf("accept failed for PORT2 with error: %d\n", WSAGetLastError());
        closesocket(listenSocket1);
        closesocket(listenSocket2);
        closesocket(clientSocket1);
        WSACleanup();
        return 1;
    }
    printf("Client 2 connected on PORT2.\n");

    // Create threads for each client
    HANDLE clientThread1 = CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket1, 0, NULL);
    HANDLE clientThread2 = CreateThread(NULL, 0, ClientHandler, (LPVOID)clientSocket2, 0, NULL);

    // Wait for threads to finish
    WaitForSingleObject(clientThread1, INFINITE);
    WaitForSingleObject(clientThread2, INFINITE);

    // Close thread handles
    CloseHandle(clientThread1);
    CloseHandle(clientThread2);

    // Cleanup
    closesocket(listenSocket1);
    closesocket(listenSocket2);
    WSACleanup();

    return 0;
}

DWORD WINAPI ClientHandler(LPVOID clientSocket)
{
    SOCKET socket = (SOCKET)clientSocket;
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    do
    {
        // Receive data until the client shuts down the connection
        iResult = recv(socket, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0)
        {
            recvbuf[iResult] = '\0'; // Null-terminate the received string
            printf("Message received from client: %s\n", recvbuf);
        }
        else if (iResult == 0)
        {
            printf("Connection with client closed.\n");
        }
        else
        {
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
