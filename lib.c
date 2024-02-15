#include "lib.h"
#include <stdio.h>

// djb2 hash function
// http://www.cse.yorku.ca/~oz/hash.html
unsigned int djb2_hash(const char* str) {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash % HASH_SIZE;
}

// Insert a key-value pair into the hash map
void map_insert(struct HashMap* map, const char* key, struct Enum* value) {
  unsigned int hashval = djb2_hash(key);
  struct HashNode* new_node = calloc(1, sizeof(struct HashNode));
  if (new_node == NULL) {
    fprintf(stderr, "Error: Memory allocation failed.\n");
    exit(1);
  }

  strcpy(new_node->key, key);
  strcpy(new_node->value.name, value->name);
  new_node->value.num_values = value->num_values;
  for (int i = 0; i < value->num_values; i++) {
    strcpy(new_node->value.values[i], value->values[i]);
  }
  new_node->next = map->table[hashval];
  map->table[hashval] = new_node;
}

// Get the value for a key from the hash map
struct Enum* map_get(struct HashMap* map, const char* key) {
  unsigned int hashval = djb2_hash(key);
  struct HashNode* current = map->table[hashval];
  while (current != NULL) {
    if (strcmp(current->key, key) == 0) {
      return &(current->value);
    }
    current = current->next;
  }
  return NULL;
}

// Initialize the hash map
void init_hash_map(struct HashMap* map) {
  for (int i = 0; i < HASH_SIZE; i++) {
    map->table[i] = NULL;
  }
}

// Free the hash map
void free_hash_map(struct HashMap* map) {
  for (int i = 0; i < HASH_SIZE; i++) {
    struct HashNode* current = map->table[i];
    while (current != NULL) {
      struct HashNode* temp = current;
      current = current->next;
      free(temp);
    }
  }
}

void camelcase(char* str) {
  if (str == NULL) {
    return;
  }

  size_t str_len = strlen(str);
  if (str_len == 0) {
    return;
  }


  char* data = str;
  int dest_index = 0;
  int capitalize = 0;

  while (data[dest_index] != '\0') {
    if (data[dest_index] == ' ' || data[dest_index] == '_') {
      capitalize = 1;
    } else if (capitalize) {
      data[dest_index] = toupper(data[dest_index]);
      capitalize = 0;
    } else {
      data[dest_index] = tolower(data[dest_index]);
    }
    dest_index++;
  }

  // Remove spaces and underscores from the string
  int j = 0;
  for (dest_index = 0; data[dest_index] != '\0'; dest_index++) {
    if (data[dest_index] != ' ' && data[dest_index] != '_') {
      data[j] = data[dest_index];
      j++;
    }
  }

  // Pascal case starts with lower case, unlike camel case.
  data[0] = tolower(data[0]);
  data[j] = '\0';
}

// case insensitive strstr
char* ci_strstr(char* haystack, const char* needle) {
  int haystack_len = strlen(haystack);
  int needle_len = strlen(needle);
  if (needle_len == 0) {
    return haystack;
  }

  for (int i = 0; i < haystack_len - needle_len + 1; i++) {
    int j;
    for (j = 0; j < needle_len; j++) {
      if (tolower(haystack[i + j]) != tolower(needle[j])) {
        break;
      }
    }
    if (j == needle_len) {
      return &haystack[i];
    }
  }

  return NULL;
}

// case insensitive sscanf
int ci_sscanf(const char* input, const char* format, ...) {
  char* input_lower = strdup(input);
  char* format_lower = strdup(format);
  if (input_lower == NULL || format_lower == NULL) {
    fprintf(stderr, "ci_sscanf(): strdup(): Error: Memory allocation failed.\n");
    return -1;
  }

  // Convert input and format to lowercase
  for (int i = 0; input_lower[i]; i++) {
    input_lower[i] = tolower(input_lower[i]);
  }
  for (int i = 0; format_lower[i]; i++) {
    format_lower[i] = tolower(format_lower[i]);
  }

  int ret;

  va_list args;
  va_start(args, format);
  ret = vsscanf(input_lower, format_lower, args);
  va_end(args);

  free(input_lower);
  free(format_lower);
  return ret;
}

void pascal_case(char* str) {
  camelcase(str);
  if (strlen(str) > 0) {
    str[0] = toupper(str[0]);
  }
}

static void write_enum_to_go_file(FILE* go_file, struct Enum* e) {
  if (e == NULL) {
    fprintf(stderr, "Error: write_enum_to_go_file(): Enum is NULL\n");
    exit(1);
  }

  char originalType[MAX_LINE_LENGTH];
  if (strlen(e->name) > MAX_LINE_LENGTH - 1) {
    fprintf(stderr, "Error: Type name %s longer than %d characters\n", e->name,
            MAX_LINE_LENGTH - 1);
    exit(1);
  }

  // Save the original type name before modifying it
  strncpy(originalType, e->name, MAX_LINE_LENGTH - 1);
  originalType[strlen(e->name)] = '\0';

  // Convert the type name to pascal case
  pascal_case(e->name);

  fprintf(go_file, "type %s string\n\n", e->name);
  fprintf(go_file, "const (\n");
  for (int i = 0; i < e->num_values; i++) {
    char pascalCaseValue[MAX_LINE_LENGTH];
    strncpy(pascalCaseValue, e->values[i], MAX_LINE_LENGTH - 1);
    pascalCaseValue[MAX_LINE_LENGTH - 1] = '\0';
    pascal_case(pascalCaseValue);
    fprintf(go_file, "  %s%s %s = \"%s\"\n", e->name, pascalCaseValue, e->name, e->values[i]);
  }

  fprintf(go_file, ")\n\n");

  // Implement the Stringer interface
  fprintf(go_file, "func (e %s) String() string {\n", e->name);
  fprintf(go_file, "  return string(e)\n");
  fprintf(go_file, "}\n\n");

  // Gorm data type as the original string()
  fprintf(go_file, "func (e %s) GormDataType() string {\n", e->name);
  fprintf(go_file, "  return \"%s\"\n", originalType);
  fprintf(go_file, "}\n\n");

  // implement function ValidValues() that returns a slice of all valid values
  fprintf(go_file, "func (e %s) ValidValues() []string {\n", e->name);
  fprintf(go_file, "  return []string{\n");
  for (int i = 0; i < e->num_values; i++) {
    fprintf(go_file, "    \"%s\",\n", e->values[i]);
  }
  fprintf(go_file, "  }\n");
  fprintf(go_file, "}\n\n");

  // implement function IsValid() that returns true if the value is valid
  fprintf(go_file, "func (e %s) IsValid() bool {\n", e->name);
  fprintf(go_file, "  for _, v := range e.ValidValues() {\n");
  fprintf(go_file, "    if string(e) == v {\n");
  fprintf(go_file, "      return true\n");
  fprintf(go_file, "    }\n");
  fprintf(go_file, "  }\n");
  fprintf(go_file, "  return false\n");
  fprintf(go_file, "}\n\n");
}

void write_enums_map_to_go_file(FILE* go_file, const char* pkg, struct HashMap* map) {
  fprintf(go_file, "package %s\n\n", pkg);

  for (size_t i = 0; i < HASH_SIZE; ++i) {
    struct HashNode* current = map->table[i];
    while (current != NULL) {
      write_enum_to_go_file(go_file, &(current->value));
      current = current->next;
    }
  }
}

void format_go_file(const char* filename) {
  // if gofmt is available, run it on the generated file
  char gofmt_command[1024];
  int n = snprintf(gofmt_command, sizeof(gofmt_command), "gofmt -w %s", filename);
  if (n < 0 || n >= (int)sizeof(gofmt_command)) {
    fprintf(stderr, "[Warn]: gofmt command failed or took to long\n");
    return;
  }

  if (system(gofmt_command) == 0) {
    printf("Generated and formated file: %s with gofmt\n", filename);
  }
}