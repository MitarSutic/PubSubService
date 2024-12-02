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
}

bool insertPubSub(HashMap* map, const char* key, PubSub value) {
    if (map == NULL || key == NULL) {
        printf("Invalid parameters to insert.\n");
        return false; // Return false if input parameters are invalid
    }

    unsigned int index = hash_functionClient(key);

    if (index >= TABLE_SIZE) {
        printf("Hash index out of bounds: %u\n", index);
        return false; // Return false if index is out of bounds
    }

    // Allocate memory for the new node
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    if (new_node == NULL) {
        printf("Memory allocation failed.\n");
        return false; // Return false if memory allocation fails
    }

    // Safely copy the key
    strncpy_s(new_node->key, sizeof(new_node->key), key, sizeof(new_node->key) - 1);
    new_node->key[sizeof(new_node->key) - 1] = '\0'; // Null-terminate the key

    new_node->value = value;
    new_node->next = NULL;

    // Insert the new node into the hash table
    if (map->table[index] == NULL) {
        map->table[index] = new_node;
    }
    else {
        new_node->next = map->table[index];
        map->table[index] = new_node;
    }

    return true; // Return true to indicate successful insertion
}

PubSub* getPubSub(HashMap* map, const char* key) {
    unsigned int index = hash_functionClient(key);
    HashNode* node = map->table[index];

    
    while (node != NULL) {
        if (strcmp(node->key, key) == 0) {
            return &node->value;  
        }
        node = node->next;
    }

    return NULL;  
}

void deletePubSub(HashMap* map, const char* key) {
    unsigned int index = hash_functionClient(key);
    HashNode* node = map->table[index];
    HashNode* prev = NULL;

    
    while (node != NULL && strcmp(node->key, key) != 0) {
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        printf("Key not found\n");
        return;
    }

    
    if (prev == NULL) {
        map->table[index] = node->next;  
    }
    else {
        prev->next = node->next;  
    }

    free(node);
}

