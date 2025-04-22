#include "core.h"
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

// Initialize the hash map
void initialize_map(HashMap* map) {
    for (int i = 0; i < HASH_SIZE; i++) {
        map->table[i] = NULL;
    }
}

// Insert a key-value pair into the hash map
void map_insert(HashMap* map, const char* key, const Enum* value) {
    // copy the enum value.
    Enum* new_value = ARENA_ALLOC(sizeof(Enum));
    memcpy(new_value, value, sizeof(Enum));

    unsigned int hashval = djb2_hash(key);
    HashNode* new_node   = ARENA_ALLOC(sizeof(HashNode));
    new_node->key        = ARENA_STRDUP(key);
    new_node->value      = new_value;
    new_node->next       = map->table[hashval];
    map->table[hashval]  = new_node;
}

// Get the value for a key from the hash map
struct Enum* map_get(HashMap* map, const char* key) {
    unsigned int hashval     = djb2_hash(key);
    struct HashNode* current = map->table[hashval];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

void camelcase(char* str) {
    if (str == NULL) {
        return;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return;
    }

    char* data     = str;
    int dest_index = 0;
    int capitalize = 0;

    while (data[dest_index] != '\0') {
        if (data[dest_index] == ' ' || data[dest_index] == '_') {
            capitalize = 1;
        } else if (capitalize) {
            data[dest_index] = toupper(data[dest_index]);
            capitalize       = 0;
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
    int needle_len   = strlen(needle);
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

// Case insensitive sscanf
int strcase_sscanf(const char* input, const char* format, ...) {
    char* input_lower  = ARENA_STRDUP(input);
    char* format_lower = ARENA_STRDUP(format);

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

    return ret;
}

void pascal_case(char* str) {
    camelcase(str);
    if (strlen(str) > 0) {
        str[0] = toupper(str[0]);
    }
}

static void sanitize_variable_name(char* str) {
    size_t str_len = strlen(str);
    if (str_len == 0) {
        return;
    }

    for (size_t i = 0; i < str_len; i++) {
        if (str[i] == ' ' || str[i] == '-') {
            str[i] = '_';
        }
    }

    // Remove all non-alphanumeric characters
    int j = 0;
    for (size_t i = 0; i < str_len; i++) {
        if (isalnum(str[i])) {
            str[j] = str[i];
            j++;
        }
    }
    str[j] = '\0';
}

static void write_enum_to_go_file(FILE* file, struct Enum* e) {
    // Preserve original name
    const char* originalType = ARENA_STRDUP(e->name);

    // Convert the type name to pascal case in-place
    pascal_case(e->name);

    // Remove spaces and non-alphanumeric characters from the type name
    sanitize_variable_name(e->name);

    // Write type to string.
    fprintf(file, "type %s string\n\n", e->name);
    fprintf(file, "const (\n");
    for (size_t i = 0; i < e->num_values; i++) {
        char* value = ARENA_STRDUP(e->values[i]);

        // Convert to PascalCase in-place.
        pascal_case(value);

        // Remove spaces and non-alphanumeric characters.
        sanitize_variable_name(value);

        fprintf(file, "  %s%s %s = \"%s\"\n", e->name, value, e->name, e->values[i]);
    }

    fprintf(file, ")\n\n");

    // Implement the Stringer interface
    fprintf(file, "func (e %s) String() string {\n", e->name);
    fprintf(file, "  return string(e)\n");
    fprintf(file, "}\n\n");

    // Gorm data type as the original string()
    fprintf(file, "func (e %s) GormDataType() string {\n", e->name);
    fprintf(file, "  return \"%s\"\n", originalType);
    fprintf(file, "}\n\n");

    // implement function ValidValues() that returns a slice of all valid values
    fprintf(file, "func (e %s) ValidValues() []string {\n", e->name);
    fprintf(file, "  return []string{\n");
    for (size_t i = 0; i < e->num_values; i++) {
        fprintf(file, "    \"%s\",\n", e->values[i]);
    }
    fprintf(file, "  }\n");
    fprintf(file, "}\n\n");

    // implement function IsValid() that returns true if the value is valid
    fprintf(file, "func (e %s) IsValid() bool {\n", e->name);
    fprintf(file, "  for _, v := range e.ValidValues() {\n");
    fprintf(file, "    if string(e) == v {\n");
    fprintf(file, "      return true\n");
    fprintf(file, "    }\n");
    fprintf(file, "  }\n");
    fprintf(file, "  return false\n");
    fprintf(file, "}\n\n");
}

void write_enums_map_to_go_file(FILE* f, const char* pkg, HashMap* map) {
    fprintf(f, "package %s\n\n", pkg);
    for (size_t i = 0; i < HASH_SIZE; ++i) {
        struct HashNode* current = map->table[i];
        while (current != NULL) {
            write_enum_to_go_file(f, current->value);
            current = current->next;
        }
    }
    fflush(f);
}

void format_go_file(const char* filename) {
    // if gofmt is available, run it on the generated file
    char gofmt_command[1024];
    int n = snprintf(gofmt_command, sizeof(gofmt_command), "gofmt -w %s", filename);
    if (n < 0 || n >= (int)sizeof(gofmt_command)) {
        perror("snprintf failed: unable to format generated go file");
        return;
    }

    if (system(gofmt_command) == 0) {
        printf("Generated and formated file: %s with gofmt\n", filename);
    }
}
