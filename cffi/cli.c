#include "cli.h"

const char* version = "1.0.0";

static void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s <sql-file> <go-file> <package>\n", program_name);
}

static FILE* open_file(const char* filename, const char* mode) {
    FILE* file = fopen(filename, mode);
    if (file == NULL) {
        perror("fopen() failed");
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    return file;
}

static void validate_args(int argc, char* argv[]) {
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("Version: %s\n", version);
        exit(EXIT_SUCCESS);
    }

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (argc != 4) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
}

void run(int argc, char* argv[]) {
    validate_args(argc, argv);

    FILE* sql_file = open_file(argv[1], "r");
    FILE* go_file  = open_file(argv[2], "w");

    HashMap map;
    initialize_map(&map);

    parse_sql_file(sql_file, &map);

    write_enums_map_to_go_file(go_file, argv[3], &map);

    fclose(sql_file);
    fclose(go_file);

    format_go_file(argv[2]);
}
