#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "core.h"

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

// Parse SQL file and populate the map with the types and their values.
void parse_sql_file(FILE* sql_file, struct HashMap* map);
#endif /* PARSER_H */
