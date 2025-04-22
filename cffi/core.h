#ifndef FC7853F6_51D7_485C_A9BF_25B11A9F28FA
#define FC7853F6_51D7_485C_A9BF_25B11A9F28FA

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"

#define MAX_LINE_LENGTH 1024
#define MAX_ENUM_VALUES 100
#define HASH_SIZE 101

typedef struct Enum {
    char* name;                     // enum name
    char* values[MAX_ENUM_VALUES];  // Array of enums options
    size_t num_values;              // Array size
} Enum;

typedef struct HashNode {
    char* key;
    Enum* value;
    struct HashNode* next;
} HashNode;

typedef struct HashMap {
    struct HashNode* table[HASH_SIZE];
} HashMap;

void map_insert(HashMap* map, const char* key, const Enum* value);
struct Enum* map_get(HashMap* map, const char* key);
void initialize_map(HashMap* map);

// convert string to camel case (in place)
void camelcase(char* str);

// convert string to pascal case (in place)
void pascal_case(char* str);

// case insensitive strstr
char* ci_strstr(char* haystack, const char* needle);

// case insensitive sscanf
int strcase_sscanf(const char* input, const char* format, ...);

// Generator
void write_enums_map_to_go_file(FILE* go_file, const char* pkg, HashMap* map);

void format_go_file(const char* filename);

#endif /* FC7853F6_51D7_485C_A9BF_25B11A9F28FA */
