#define TABLE_SIZE 100

typedef struct Entry {
    char *key;
    void *value;
    struct Entry *next;
} Entry;

typedef struct HashMap {
    Entry *table[TABLE_SIZE];
} HashMap;


HashMap *create_hashmap();
void insert(HashMap *map, const char *key, void *value);
void *get(HashMap *map, const char *key, int *found);
void delete_key(HashMap *map, const char *key);
void free_hashmap(HashMap *map, void (*free_value)(void *));
void iterate(HashMap *map, void (*callback)(const char *, void *));
void iterateM(HashMap *map, const char * msg, void (*callback)(const char *, void *, const char *));
