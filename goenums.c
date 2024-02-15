#include "lib.h"

enum State {
  INIT,
  INSIDE_CREATE_TYPE,
  INSIDE_ENUM_VALUES,
  INSIDE_ALTER_TYPE,

  // Comments
  DOBLE_DASH_COMMENT,
  SLASH_STAR_COMMENT,
  DOUBLE_SLASH_COMMENT,
};

enum AlterTypeState {
  ADD,
  RENAME,
  DROP,
};

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <sql-file> <go-file> <package>\n", argv[0]);
    return 1;
  }

  FILE* sql_file = fopen(argv[1], "r");
  FILE* go_file = fopen(argv[2], "w");
  if (sql_file == NULL || go_file == NULL) {
    perror("error opening file");
    return 1;
  }

  struct HashMap map;
  init_hash_map(&map);

  struct Enum current_enum;
  enum State state = INIT;
  char current_statement[MAX_LINE_LENGTH] = "";

  fprintf(go_file, "package %s\n\n", argv[3]);

  char c;
  while ((c = fgetc(sql_file)) != EOF) {
    strncat(current_statement, &c, 1);
    switch (state) {
      case INIT: {
        if (c == 'C' || c == 'c') {
          state = INSIDE_CREATE_TYPE;
          current_statement[0] = 'C';
          current_statement[1] = '\0';
        } else if (c == 'A' || c == 'a') {
          state = INSIDE_ALTER_TYPE;
          current_statement[0] = 'A';
          current_statement[1] = '\0';
        } else if (c == '-' || c == '/') {
          // peek at the next character to see if it's a comment
          // only if we are not at the end of the file
          if (feof(sql_file)) {
            break;
          }
          char next_char = fgetc(sql_file);
          if (next_char == '-') {
            state = DOBLE_DASH_COMMENT;
          } else if (next_char == '*') {
            state = SLASH_STAR_COMMENT;
          } else if (next_char == '/') {
            state = DOUBLE_SLASH_COMMENT;
          } else {
            fprintf(stderr, "Error: Invalid comment or sql statement: %s\n", current_statement);
            exit(1);
          }
          ungetc(next_char, sql_file);  // put the character back
        }
      } break;
      case DOBLE_DASH_COMMENT:
      case DOUBLE_SLASH_COMMENT: {
        while ((fgetc(sql_file)) != '\n') {
          // discard the comment
        }
        state = INIT;
      } break;
      case SLASH_STAR_COMMENT: {
        char prev_char = '\0';
        char comment[1024];
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
          strncat(comment, &next, 1);
        }
        state = INIT;
      } break;
      case INSIDE_CREATE_TYPE:
      case INSIDE_ALTER_TYPE:
        if (ci_strstr(current_statement, "CREATE TYPE ") != NULL ||
            ci_strstr(current_statement, "ALTER TYPE ") != NULL) {
          state = INSIDE_ENUM_VALUES;
          current_enum.num_values = 0;

          // Extract type name, which is the next word after "CREATE TYPE" or "ALTER TYPE"
          char type_name[MAX_LINE_LENGTH];
          int i = 0;
          while (((c = fgetc(sql_file)) != ' ') && i < MAX_LINE_LENGTH - 1) {
            type_name[i++] = c;
            strncat(current_statement, &c, 1);
          }
          type_name[i] = '\0';
          // also copies the null terminator
          strncpy(current_enum.name, type_name, sizeof(current_enum.name));

          if (i > 0 && i < MAX_LINE_LENGTH - 1) {
            strncat(current_statement, &c, 1);  // add the space after the type name
          }
        }
        break;
      case INSIDE_ENUM_VALUES:
        if (c == '\'') {
          char value[MAX_LINE_LENGTH];
          int i = 0;
          while ((c = fgetc(sql_file)) != '\'') {
            value[i++] = c;
            strncat(current_statement, &c, 1);
          }
          value[i] = '\0';
          if (i > 0 && i < MAX_LINE_LENGTH - 1) {
            strncat(current_statement, &c, 1);  // add the closing quote
          }
          strcpy(current_enum.values[current_enum.num_values++], value);
        } else if (c == ';') {
          state = INIT;

          enum AlterTypeState alterState = ADD;
          if (ci_strstr(current_statement, "ADD")) {
            alterState = ADD;
          } else if (ci_strstr(current_statement, "RENAME")) {
            alterState = RENAME;
          } else if (ci_strstr(current_statement, "DROP")) {
            alterState = DROP;
          }

          if (current_enum.num_values <= 0) {
            fprintf(stderr, "Error: No enum values found for type %s\n", current_enum.name);
            exit(1);
          }

          // check if the enum already exists in the map
          struct Enum* existing_enum = get(&map, current_enum.name);

          // does not exist, insert it into the map and continue
          if (existing_enum == NULL) {
            insert(&map, current_enum.name, &current_enum);
            continue;
          }

          switch (alterState) {
            case ADD: {
              // if it does, append the new values to the existing enum
              // skip duplicates
              for (int i = 0; i < current_enum.num_values; i++) {
                int duplicate = 0;
                for (int j = 0; j < existing_enum->num_values; j++) {
                  if (strcmp(current_enum.values[i], existing_enum->values[j]) == 0) {
                    duplicate = 1;
                    break;
                  }
                }

                if (!duplicate) {
                  strcpy(existing_enum->values[existing_enum->num_values++],
                         current_enum.values[i]);
                }
              }
            } break;
            case RENAME: {
              char old_name[MAX_LINE_LENGTH];
              char new_name[MAX_LINE_LENGTH];

              int n = ci_sscanf(current_statement, "ALTER TYPE %*s RENAME VALUE %s TO %s", old_name,
                                new_name);
              if (n != 2) {
                fprintf(stderr, "Error: Could not parse RENAME VALUE statement\n");
                fprintf(stderr, "current_statement: %s\n", current_statement);
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

              printf("Renaming value %s to %s in enum %s\n", old_name, new_name,
                     existing_enum->name);

              for (int i = 0; i < existing_enum->num_values; i++) {
                if (strcmp(existing_enum->values[i], old_name) == 0) {
                  strcpy(existing_enum->values[i], new_name);
                  break;
                }
              }
            } break;
            case DROP: {
              char value_to_drop[MAX_LINE_LENGTH];
              int n = ci_sscanf(current_statement, "ALTER TYPE %*s DROP VALUE %s", value_to_drop);
              if (n != 1) {
                fprintf(stderr, "Error: Could not parse DROP VALUE statement\n");
                exit(1);
              }

              // remove the trailing quote and semicolon
              value_to_drop[strlen(value_to_drop) - 2] = '\0';
              // overwrite opening quote
              memmove(value_to_drop, value_to_drop + 1, strlen(value_to_drop));

              printf("Dropping value %s from enum %s\n", value_to_drop, existing_enum->name);
              for (int i = 0; i < existing_enum->num_values; i++) {
                if (strcmp(existing_enum->values[i], value_to_drop) == 0) {
                  for (int j = i; j < existing_enum->num_values - 1; j++) {
                    strcpy(existing_enum->values[j], existing_enum->values[j + 1]);
                  }
                  existing_enum->num_values--;
                  break;
                }
              }
            } break;
          }
        }
        break;
    }
  }

  // Write the enums to the go file
  write_enums_map_to_go_file(go_file, &map);

  // Free the hash map
  free_hash_map(&map);

  // Close the files
  fclose(sql_file);
  fclose(go_file);

  format_go_file(argv[2]);
  return 0;
}
