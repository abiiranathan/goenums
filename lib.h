#ifndef FC7853F6_51D7_485C_A9BF_25B11A9F28FA
#define FC7853F6_51D7_485C_A9BF_25B11A9F28FA

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024
#define MAX_ENUM_VALUES 100
#define HASH_SIZE 101

struct Enum {
  char name[MAX_LINE_LENGTH];
  char values[MAX_ENUM_VALUES][MAX_LINE_LENGTH];
  int num_values;
};

struct HashNode {
  char key[MAX_LINE_LENGTH];
  struct Enum value;
  struct HashNode* next;
};

struct HashMap {
  struct HashNode* table[HASH_SIZE];
};

void map_insert(struct HashMap* map, const char* key, struct Enum* value);
struct Enum* map_get(struct HashMap* map, const char* key);
void init_hash_map(struct HashMap* map);
void free_hash_map(struct HashMap* map);

// convert string to camel case (in place)
void camelcase(char* str);

// convert string to pascal case (in place)
void pascal_case(char* str);

// case insensitive strstr
char* ci_strstr(char* haystack, const char* needle);

// case insensitive sscanf
int ci_sscanf(const char* input, const char* format, ...);


// Generator
void write_enums_map_to_go_file(FILE* go_file, const char* pkg, struct HashMap* map);

void format_go_file(const char* filename);

#endif /* FC7853F6_51D7_485C_A9BF_25B11A9F28FA */
