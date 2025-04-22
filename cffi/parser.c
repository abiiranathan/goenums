#include "parser.h"
#include <stdlib.h>

// Starting state for the state machine
static void state_init(char c, enum State* state, char* cur_stmt, FILE* sql_file) {
    if (c == 'C' || c == 'c') {
        *state      = INSIDE_CREATE_TYPE;
        cur_stmt[0] = 'C';
        cur_stmt[1] = '\0';
    } else if (c == 'A' || c == 'a') {
        *state      = INSIDE_ALTER_TYPE;
        cur_stmt[0] = 'A';
        cur_stmt[1] = '\0';
    } else if (c == '-' || c == '/') {
        // peek at the next character to see if it's a comment
        if (feof(sql_file)) {
            return;
        }

        char next_char = fgetc(sql_file);  // consume the next character
        if (next_char == '-') {
            *state = DOBLE_DASH_COMMENT;  // double dash comment (--)
        } else if (next_char == '*') {
            *state = SLASH_STAR_COMMENT;  // slash star comment (/*)
        } else if (next_char == '/') {
            *state = DOUBLE_SLASH_COMMENT;  // double slash comment (//)
        } else {
            fprintf(stderr, "Error: Invalid comment or sql statement: %s\n", cur_stmt);
            exit(EXIT_FAILURE);
        }

        ungetc(next_char, sql_file);  // put the character back
    }
}

// State for /* comments */
static void state_slash_star_comment(enum State* state, FILE* sql_file) {
    char prev_char = '\0';
    while (1) {
        char next = fgetc(sql_file);
        if (next == EOF) {
            fprintf(stderr, "Error: Unterminated comment\n");
            exit(1);
        }

        if (prev_char == '*' && next == '/') {
            break;
        }
        prev_char = next;
    }

    // go back to the initial state
    *state = INIT;
}

// State for CREATE TYPE and ALTER TYPE statements
static void state_create_or_alter_type(char c, enum State* state, char* cur_stmt, FILE* sql_file,
                                       struct Enum* cur_enum) {
    if (ci_strstr(cur_stmt, "CREATE TYPE ") || ci_strstr(cur_stmt, "ALTER TYPE ")) {
        *state               = INSIDE_ENUM_VALUES;
        cur_enum->num_values = 0;

        // Extract type name, which is the next word after "CREATE TYPE" or "ALTER TYPE"
        char type_name[MAX_LINE_LENGTH];
        int i = 0;
        while (((c = fgetc(sql_file)) != ' ') && i < MAX_LINE_LENGTH - 1) {
            type_name[i++] = c;
            strncat(cur_stmt, &c, 1);
        }
        type_name[i] = '\0';  // terminate the string

        cur_enum->name = ARENA_STRDUP(type_name);

        // add the space after the type name in the current statement
        if (i > 0) {
            if (i < MAX_LINE_LENGTH - 1) {
                strncat(cur_stmt, &c, 1);
            } else {
                fprintf(stderr, "Current statement exceeds %d bytes: %s\n", MAX_LINE_LENGTH, cur_stmt);
            }
        }
    }
}

// State for enum values
static void state_inside_enum_values(char c, enum State* state, char* cur_stmt, FILE* sql_file,
                                     struct Enum* current_enum, struct HashMap* map) {
    if (c == '\'') {
        // extract the enum values from the current sql statement
        char value[MAX_LINE_LENGTH];
        int i = 0;
        while ((c = fgetc(sql_file)) != '\'') {
            value[i++] = c;
            strncat(cur_stmt, &c, 1);
        }
        value[i] = '\0';
        if (i > 0 && i < MAX_LINE_LENGTH - 1) {
            strncat(cur_stmt, &c, 1);  // add the closing single quote
        }
        current_enum->values[current_enum->num_values++] = ARENA_STRDUP(value);
    } else if (c == ';') {
        *state = INIT;  // go back to the initial state

        enum AlterTypeState alterState = ADD;
        if (ci_strstr(cur_stmt, "ADD")) {
            alterState = ADD;
        } else if (ci_strstr(cur_stmt, "RENAME")) {
            alterState = RENAME;
        } else if (ci_strstr(cur_stmt, "DROP")) {
            alterState = DROP;
        } else {
            // if the statement is not an ADD, RENAME or DROP, ignore it
            // we may be inside CREATE TYPE... AS ENUM
        }

        if (current_enum->num_values <= 0) {
            fprintf(stderr, "Error: No enum values found for type %s\n", current_enum->name);
            exit(1);
        }

        // check if the enum already exists in the map
        struct Enum* existing_enum = map_get(map, current_enum->name);

        // does not exist, insert it into the map and return
        if (existing_enum == NULL) {
            map_insert(map, current_enum->name, current_enum);
            return;
        }

        switch (alterState) {
            case ADD: {
                // if it does, append the new values to the existing enum
                // skip duplicates
                for (size_t i = 0; i < current_enum->num_values; i++) {
                    int duplicate = 0;
                    for (size_t j = 0; j < existing_enum->num_values; j++) {
                        if (strcmp(current_enum->values[i], existing_enum->values[j]) == 0) {
                            duplicate = 1;
                            break;
                        }
                    }

                    if (!duplicate) {
                        existing_enum->values[existing_enum->num_values++] = current_enum->values[i];
                    }
                }
            } break;
            case RENAME: {
                char old_name[MAX_LINE_LENGTH];
                char new_name[MAX_LINE_LENGTH];

                int n = strcase_sscanf(cur_stmt, "ALTER TYPE %*s RENAME VALUE %s TO %s", old_name, new_name);
                if (n != 2) {
                    fprintf(stderr, "Error: Could not parse RENAME VALUE statement\n");
                    fprintf(stderr, "current_statement: %s\n", cur_stmt);
                    exit(1);
                }

                // remove the trailing quote from old_name
                old_name[strlen(old_name) - 1] = '\0';
                // overwrite opening quote
                memmove(old_name, old_name + 1, strlen(old_name));

                // remove the trailing quote from new_name and semicolon
                new_name[strlen(new_name) - 2] = '\0';
                // overwrite opening quote
                memmove(new_name, new_name + 1, strlen(new_name));

                printf("Renaming value %s to %s in enum %s\n", old_name, new_name, existing_enum->name);

                for (size_t i = 0; i < existing_enum->num_values; i++) {
                    if (strcmp(existing_enum->values[i], old_name) == 0) {
                        existing_enum->values[i] = ARENA_STRDUP(new_name);
                        break;
                    }
                }
            } break;
            case DROP: {
                char value_to_drop[MAX_LINE_LENGTH];
                int n = strcase_sscanf(cur_stmt, "ALTER TYPE %*s DROP VALUE %s", value_to_drop);
                if (n != 1) {
                    fprintf(stderr, "Error: Could not parse DROP VALUE statement\n");
                    exit(1);
                }

                // remove the trailing quote and semicolon
                value_to_drop[strlen(value_to_drop) - 2] = '\0';
                // overwrite opening quote
                memmove(value_to_drop, value_to_drop + 1, strlen(value_to_drop));

                printf("Dropping value %s from enum %s\n", value_to_drop, existing_enum->name);
                for (size_t i = 0; i < existing_enum->num_values; i++) {
                    if (strcmp(existing_enum->values[i], value_to_drop) == 0) {
                        for (size_t j = i; j < existing_enum->num_values - 1; j++) {
                            existing_enum->values[j] = existing_enum->values[j + 1];
                        }
                        existing_enum->num_values--;
                        break;
                    }
                }
            } break;
        }
    }
}

void parse_sql_file(FILE* sql_file, struct HashMap* map) {
    Enum current_enum;
    current_enum.name       = NULL;
    current_enum.num_values = 0;
    memset(current_enum.values, 0, sizeof(char*) * MAX_ENUM_VALUES);

    enum State state                        = INIT;
    char current_statement[MAX_LINE_LENGTH] = "";

    char c;
    while ((c = fgetc(sql_file)) != EOF) {
        strncat(current_statement, &c, 1);
        switch (state) {
            case INIT: {
                state_init(c, &state, current_statement, sql_file);
            } break;
            case DOBLE_DASH_COMMENT:
            case DOUBLE_SLASH_COMMENT: {
                // consume the rest of the line
                while ((fgetc(sql_file)) != '\n') {
                    // discard the comment
                }
                state = INIT;
            } break;
            case SLASH_STAR_COMMENT: {
                state_slash_star_comment(&state, sql_file);
            } break;
            case INSIDE_CREATE_TYPE:
            case INSIDE_ALTER_TYPE: {
                state_create_or_alter_type(c, &state, current_statement, sql_file, &current_enum);
            } break;
            case INSIDE_ENUM_VALUES: {
                state_inside_enum_values(c, &state, current_statement, sql_file, &current_enum, map);
            } break;
        }
    }
}
