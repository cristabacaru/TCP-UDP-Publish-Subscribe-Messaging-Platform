#ifndef MAP_H
#define MAP_H


// pair of string key and integer value
struct pairStruct {
    char key[11];
    void* value;
    int size;
    int capacity;
};

// array-based map
struct mapStruct {
    struct pairStruct* data;
    int size;
    int capacity;
};

// initialize a new map
// returns a pointer to the new map
struct mapStruct* map_init(void);

// retrieve a key-value pair by key
// returns the pair if found, or NULL if there's no match
struct pairStruct* map_get(const struct mapStruct *map, const char key[11]);


// check if a topic is subscribed to
// returns 1 if match found, 0 if not
int is_subscribed_no_wildcard(struct pairStruct* current_pair, char topic[51]);

// check if a topic is subscribed to considering wildcards
// returns 1 if match found, 0 if not
int is_subscribed_wildcard(struct pairStruct* current_pair, char topic[51]);

// add a new key-value(int) pair
// returns 0 on success, 1 if key already exists
int map_add_int(struct mapStruct *map, const char key[11], int value);

// add a string to the array of strings under a specific key
// returns 0 on success
int map_add_string_to_array(struct mapStruct *map, char key[10], char* str);

// remove a key-value pair by key
void map_remove(struct mapStruct *map, const char key[11]);

// remove a specific topic from the list of topics for a given key
void remove_topic(struct mapStruct *map, char key[10], char* topic);

// free the entire map
void map_free(struct mapStruct *map);

#endif
