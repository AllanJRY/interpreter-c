
#ifndef INTERP_TABLE_H

#include "common.h"
#include "value.h"

typedef struct Table_Entry {
    Obj_String* key;
    Value       value;
} Table_Entry;

typedef struct Table {
    int          count;
    int          cap;
    Table_Entry* entries;
} Table;

void table_init(Table* table);
void table_free(Table* table);
bool table_get(Table* table, Obj_String* key, Value* value);
Obj_String* table_find_string(Table* table, const char* chars, int length, uint32_t hash);
bool table_set(Table* table, Obj_String* key, Value value);
bool table_delete(Table* table, Obj_String* key);
void table_copy(Table* from, Table* to);

#define INTERP_TABLE_H
#endif
