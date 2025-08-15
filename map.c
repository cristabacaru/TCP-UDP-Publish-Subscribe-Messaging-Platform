#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

// struct to hold key-value pairs in the map
struct pairStruct {
    char key[11];      // key for the pair
    void* value;
    int size;          // current size of the value (like number of strings in the array)
    int capacity;      // total capacity of the value array (if value is an array)
};

// struct to hold the map (array of pairs)
struct mapStruct {
    struct pairStruct* data;  // array of key-value pairs
    int size;                 // number of key-value pairs in the map
    int capacity;             // current capacity of the map
};

// initialize a new map
struct mapStruct* map_init(void) {
    struct mapStruct *map = malloc(sizeof(struct mapStruct));  // allocate memory for the map
    if (!map) {
        fprintf(stderr, "Error with malloc\n");
        exit(1);
    }

    map->size = 0;
    map->capacity = 3;  // initial capacity
    map->data = malloc(map->capacity * sizeof(struct pairStruct));  // allocate memory for the pairs
    if (!map->data) {
        fprintf(stderr, "Error with malloc\n");
        exit(1);
    }

    return map;  // return the initialized map
}

// function to get a pair by key from the map
struct pairStruct* map_get(struct mapStruct *map, char key[10]) {
    for (int i = 0; i < map->size; i++) {
        if (strcmp(map->data[i].key, key) == 0)  // compare keys
            return &map->data[i];  // return the pair if found
    }
    return NULL;  // return NULL if the key is not found
}

// function to check if a topic is subscribed without using wildcards
int is_subscribed_no_wildcard(struct pairStruct* current_pair, char topic[51]) {
    char **array = (char **)current_pair->value;
    for (int j = 0; j < current_pair->size; j++) {
        if (strcmp(array[j], topic) == 0)
            return 1;
    }
    return 0;  // not found
}

// function to check if a topic is subscribed using wildcards
int is_subscribed_wildcard(struct pairStruct* current_pair, char topic[51]) {
    char **array = (char **)current_pair->value;
    for (int j = 0; j < current_pair->size; j++) {
        if (find_match(topic, array[j]) == true)
            return 1;
    }
    return 0;  // no match found
}

// function to add an integer value to the map
int map_add_int(struct mapStruct *map, char key[10], int value) {
    if (map_get(map, key)) return 1;  // if the key already exists, return 1

    if (map->size == map->capacity) {  // if map is full, resize it
        map->capacity *= 2;
        map->data = realloc(map->data, map->capacity * sizeof(struct pairStruct));  // realloc to double capacity
        if (!map->data) exit(1);
    }

    strcpy(map->data[map->size].key, key);  // copy key into the new pair
    int *ptr = malloc(sizeof(int));  // allocate memory for the integer value
    *ptr = value;
    map->data[map->size].value = ptr;  // store pointer to the value
    map->data[map->size].size = 1;  // (1 because its a single int)
    map->data[map->size].capacity = 1;  // (1 because its a single int)
    map->size++;  // increment map size
    return 0;
}

// function to add a string to an array of strings in the map
int map_add_string_to_array(struct mapStruct *map, char key[10], char str[51]) {
    struct pairStruct* pair = map_get(map, key);  // get the pair for the key
    if (pair) {  // if the pair exists, add the string to the array
        char **array = (char **)pair->value;

        if (pair->size == pair->capacity) {  // if the array is full, resize it
            pair->capacity *= 2;
            array = realloc(array, pair->capacity * sizeof(char*));  // realloc to double capacity
            if (!array) {
                fprintf(stderr, "Error with realloc\n");
                exit(1);
            }
            pair->value = array;
        }

        array[pair->size] = malloc((50 + 1) * sizeof(char));  // allocate memory for the new string
        memcpy(array[pair->size], str, strlen(str));  // copy the string
        array[pair->size][strlen(str)] = '\0';
        pair->size++;  // increment size of the array
        return 0;
    }

    // if the pair doesn't exist, create a new pair
    if (map->size == map->capacity) {  // if map is full, resize it
        map->capacity *= 2;
        map->data = realloc(map->data, map->capacity * sizeof(struct pairStruct));
        if (!map->data) exit(1);
    }

    strcpy(map->data[map->size].key, key);  // copy key into the new pair
    char **array = malloc(8 * sizeof(char*));  // allocate initial array space for strings
    array[0] = malloc((50 + 1) * sizeof(char));  // allocate space for the first string
    memcpy(array[0], str, strlen(str));  // copy the string into the array
    array[0][strlen(str)] = '\0';

    map->data[map->size].value = array;
    map->data[map->size].size = 1;
    map->data[map->size].capacity = 8;
    map->size++;  // increment map size
    return 0;
}

// function to remove a topic from the map
void remove_topic(struct mapStruct *map, char key[10], char* topic) {
    struct pairStruct* pair = map_get(map, key);
    if (!pair) return;  // if no pair found, return

    char **values_array = (char **)pair->value;
    for (int i = 0; i < pair->size; i++) {
        if (strcmp(values_array[i], topic) == 0) {  // if the topic matches, remove it
            free(values_array[i]);  // free the removed string

            // shift the rest of the array left to fill the gap
            for (int j = i; j < pair->size - 1; j++) {
                values_array[j] = values_array[j + 1];
            }

            pair->size--;  // decrement the array size
            return;
        }
    }
}

// function to remove a key-value pair from the map
void map_remove(struct mapStruct *map, char key[10]) {
    for (int i = 0; i < map->size; i++) {
        if (strcmp(map->data[i].key, key) == 0) {  // find the pair by key
            // shift the rest of the pairs left to fill the gap
            for (int j = i; j < map->size - 1; j++) {
                map->data[j] = map->data[j + 1];
            }
            map->size--;  // decrement map size
            return;
        }
    }
}

// function to free the memory used by the map
void map_free(struct mapStruct *map) {
    for (int i = 0; i < map->size; i++) {
        struct pairStruct* pair = &map->data[i];
        if (pair->capacity == 1 && pair->size == 1) {
            // if its an int, free the allocated space
            free(pair->value);
        } else {
            // if its an array of strings, free each string in the array
            char **array = (char **)pair->value;
            for (int j = 0; j < pair->size; j++) {
                free(array[j]);
            }
            free(array);  // free the string array itself
        }
    }
    free(map->data);  // free the map data array
    free(map);  // free the map structure
}
