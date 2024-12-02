#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WinSock2.h>
#include <Windows.h>

#define TABLE_SIZE 50  


typedef struct PubSub {
    char topic[256];        
    char info[256];         
    SOCKET accepted_socket;    
} PubSub;


typedef struct HashNode {
    char key[256];           
    PubSub value;            
    struct HashNode* next;   
} HashNode;

typedef struct HashMap {
    HashNode* table[TABLE_SIZE];  
} HashMap;
