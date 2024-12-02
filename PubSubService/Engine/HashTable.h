#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WinSock2.h>
#include <Windows.h>
#include "Structures.h"

unsigned int hash_functionClient(const char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key;  
        key++;
    }
    return hash % TABLE_SIZE;
}

void initialize_hashmapClient(HashMap* map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        map->table[i] = NULL;
    }
    InitializeCriticalSection(&map->cs);
}

bool insertPubSub(HashMap* map, const char* key, PubSub value) {
    if (map == NULL || key == NULL) {
        printf("Invalid parameters to insert.\n");
        return false;
    }

    unsigned int index = hash_functionClient(key);

    if (index >= TABLE_SIZE) {
        printf("Hash index out of bounds: %u\n", index);
        return false;
    }

    // Enter critical section to protect the hashmap
    EnterCriticalSection(&map->cs);

    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (new_node == NULL) {
        printf("Memory allocation failed.\n");
        LeaveCriticalSection(&map->cs);  // Leave critical section before returning
        return false;
    }

    strncpy_s(new_node->key, sizeof(new_node->key), key, sizeof(new_node->key) - 1);
    new_node->key[sizeof(new_node->key) - 1] = '\0';

    new_node->value = value;
    new_node->next = NULL;

    if (map->table[index] == NULL) {
        map->table[index] = new_node;
    }
    else {
        new_node->next = map->table[index];
        map->table[index] = new_node;
    }

    LeaveCriticalSection(&map->cs);  // Leave critical section

    return true;
}

PubSub* getPubSub(HashMap* map, const char* key) {
    unsigned int index = hash_functionClient(key);
    HashNode* node = map->table[index];

    // Enter critical section to protect the hashmap
    EnterCriticalSection(&map->cs);

    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            LeaveCriticalSection(&map->cs);  // Leave critical section before returning
            return &node->value;
        }
        node = node->next;
    }

    LeaveCriticalSection(&map->cs);  // Leave critical section before returning
    return NULL;
}

void deletePubSub(HashMap* map, const char* key) {
    unsigned int index = hash_functionClient(key);
    HashNode* node = map->table[index];
    HashNode* prev = NULL;

    // Enter critical section to protect the hashmap
    EnterCriticalSection(&map->cs);

    while (node != NULL && strcmp(node->key, key) != 0) {
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        printf("Key not found\n");
        LeaveCriticalSection(&map->cs);  // Leave critical section before returning
        return;
    }

    if (prev == NULL) {
        map->table[index] = node->next;
    }
    else {
        prev->next = node->next;
    }

    free(node);

    LeaveCriticalSection(&map->cs);  // Leave critical section
}

MessageNode* createNode(const char* message) {
    MessageNode* newNode = (MessageNode*)malloc(sizeof(MessageNode));
    if (newNode == NULL) {
        printf("Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    strncpy_s(newNode->message, message, MAX_MESSAGE_LENGTH - 1);
    newNode->message[MAX_MESSAGE_LENGTH - 1] = '\0';  // Ensure null-termination
    newNode->next = NULL;
    return newNode;
}

void addMessage(MessageNode** head, const char* message) {
    MessageNode* newNode = createNode(message);

    // If the list is empty
    if (*head == NULL) {
        *head = newNode;
    }
    else {
        // Traverse to the end of the list
        MessageNode* temp = *head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newNode;  // Append the new node
    }
}

void printMessages(MessageNode* head) {
    MessageNode* temp = head;
    while (temp != NULL) {
        printf("%s\n", temp->message);
        temp = temp->next;
    }
}

void freeMessages(MessageNode* head) {
    MessageNode* temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp);
    }
}



void initialize_hashmap(HashMapOfSubscribers* map) {
    memset(map->entries, 0, sizeof(map->entries));
    InitializeCriticalSection(&map->cs);
}

// Simple hash function
unsigned int hash(const char* str) {
    unsigned int hashValue = 0;
    while (*str) {
        hashValue = (hashValue * 31) + *str++;
    }
    return hashValue % TABLE_SIZE;  // Hash map size of 256
}

// Insert a message into the hashmap under the given topic
void insert_message(HashMapOfSubscribers* map, const char* topic, const char* message) {
    unsigned int index = hash(topic);
    HashMapSubscriber* entry = map->entries[index];

    // Enter critical section to protect the hashmap
    EnterCriticalSection(&map->cs);

    while (entry) {
        if (strcmp(entry->topic, topic) == 0) {
            MessageNode* newNode = (MessageNode*)malloc(sizeof(MessageNode));
            strncpy_s(newNode->message, message, MAX_MESSAGE_LENGTH);
            newNode->next = entry->messages;
            entry->messages = newNode;
            LeaveCriticalSection(&map->cs);  // Leave critical section before returning
            return;
        }
        entry = entry->next;
    }

    entry = (HashMapSubscriber*)malloc(sizeof(HashMapSubscriber));
    strncpy_s(entry->topic, topic, MAX_TOPIC_LEN);
    entry->messages = NULL;
    entry->next = map->entries[index];
    map->entries[index] = entry;

    MessageNode* newNode = (MessageNode*)malloc(sizeof(MessageNode));
    strncpy_s(newNode->message, message, MAX_MESSAGE_LENGTH);
    newNode->next = entry->messages;
    entry->messages = newNode;

    LeaveCriticalSection(&map->cs);  // Leave critical section
}

// Retrieve the list of messages for a given topic
MessageNode* get_messages(HashMapOfSubscribers* map, const char* topic) {
    unsigned int index = hash(topic);
    HashMapSubscriber* entry = map->entries[index];

    // Enter critical section to protect the hashmap
    EnterCriticalSection(&map->cs);

    while (entry) {
        if (strcmp(entry->topic, topic) == 0) {
            LeaveCriticalSection(&map->cs);  // Leave critical section before returning
            return entry->messages;
        }
        entry = entry->next;
    }

    LeaveCriticalSection(&map->cs);  // Leave critical section before returning
    return NULL;
}

// Print all messages in a linked list
void print_messages(MessageNode* node) {
    while (node) {
        printf("Message: %s\n", node->message);
        node = node->next;
    }
}

// Free memory used by the hashmap
void free_hashmap(HashMapOfSubscribers* map) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashMapSubscriber* entry = map->entries[i];
        while (entry) {
            MessageNode* node = entry->messages;
            while (node) {
                MessageNode* temp = node;
                node = node->next;
                free(temp);
            }
            HashMapSubscriber* tempEntry = entry;
            entry = entry->next;
            free(tempEntry);
        }
    }
    DeleteCriticalSection(&map->cs);  // Delete critical section
}