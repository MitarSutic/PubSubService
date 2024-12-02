#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WinSock2.h>
#include <Windows.h>

#define TABLE_SIZE 50 
#define MAX_MESSAGE_LENGTH 128
#define MAX_TOPIC_LEN 128


typedef struct PubSub {
    char topic[256];        
    char info[256];         
    SOCKET accepted_socket;    
} PubSub;

// za povezane klijente
typedef struct HashNode {
    char key[256];           
    PubSub value;            
    struct HashNode* next;   
} HashNode;

// recnik klijenata
typedef struct HashMap {
    HashNode* table[TABLE_SIZE];
    CRITICAL_SECTION cs;
} HashMap;

typedef struct MessageNode 
{
    char message[MAX_MESSAGE_LENGTH];
    struct MessageNode* next;
}MessageNode;

typedef struct SubscriberMessage
{
    char topic[256];
    MessageNode* messages;
};

// struktura za subscribera

typedef struct HashMapSubscriber {
    char topic[MAX_TOPIC_LEN];
    MessageNode* messages;
    struct HashMapSubscriber* next;
} HashMapSubscriber;

// recnik subscribera
typedef struct HashMapOfSubscribers {
    HashMapSubscriber* entries[TABLE_SIZE]; 
    CRITICAL_SECTION cs;
} HashMapOfSubscribers;