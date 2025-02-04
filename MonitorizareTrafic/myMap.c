#include "myMap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


unsigned int hash(const char *key) {
    unsigned long int hash_value = 0;
    int i = 0;

    while (key[i] != '\0') {
        hash_value = (hash_value << 5) + key[i];
        i++;
    }

    return hash_value % TABLE_SIZE;
}

HashMap *create_hashmap() {
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    if (!map) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        map->table[i] = NULL;
    }
    return map;
}

Entry *create_entry(const char *key, void *value) {
    Entry *entry = (Entry *)malloc(sizeof(Entry));
    if (!entry) {
        fprintf(stderr, "Memory allocation error\n");
        return NULL;
    }
    entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;
    return entry;
}

void insert(HashMap *map, const char *key, void *value) {
    unsigned int index = hash(key);
    Entry *current = map->table[index];
    printf("{HashMap} Inserting at hash [%d].\n", index);
    
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;
            return;
        }
        current = current->next;
    }

    Entry *new_entry = create_entry(key, value);
    if (!new_entry) return;

    new_entry->next = map->table[index];
    map->table[index] = new_entry;
}

void *get(HashMap *map, const char *key, int *found) {
    unsigned int index = hash(key);
    Entry *current = map->table[index];
    printf("{HashMap} Comparing start at hash [%d].\n", index);
    
    while (current != NULL) {
    printf("{HashMap} Comparing [%s] with [%s].\n", current->key, key);
        if (strcmp(current->key, key) == 0) {
            *found = 1;
            return current->value;
        }
        current = current->next;
    }

    *found = 0;
    return NULL;
}

void delete_key(HashMap *map, const char *key) {
    unsigned int index = hash(key);
    Entry *current = map->table[index];
    Entry *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                map->table[index] = current->next;
            } else {
                prev->next = current->next;
            }

            free(current->key);
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void free_hashmap(HashMap *map, void (*free_value)(void *)) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *current = map->table[i];
        while (current != NULL) {
            Entry *temp = current;
            current = current->next;

            free(temp->key);
            if (free_value) {
                free_value(temp->value);
            }
            free(temp);
        }
    }
    free(map);
}
void iterate(HashMap *map, void (*callback)(const char *, void *)) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *current = map->table[i];
        while (current != NULL) {
            callback(current->key, current->value);
            current = current->next;
        }
    }
}
void iterateM(HashMap *map, const char * msg, void (*callback)(const char *, void *, const char *)) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Entry *current = map->table[i];
        while (current != NULL) {
            printf("{HashMap} Got iteration item [%s] with msg [%s].\n", current->key, msg);
            callback(current->key, current->value, msg);
            current = current->next;
        }
    }
}
