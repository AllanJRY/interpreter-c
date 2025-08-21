#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

static Table_Entry* _entry_find(Table_Entry* entries, int cap, Obj_String* key);
static void _table_adjust_cap(Table* table, int cap);

void table_init(Table* table) {
    table->count   = 0;
    table->cap     = 0;
    table->entries = NULL;
}

void table_free(Table* table) {
    FREE_ARRAY(Table_Entry, table->entries, table->cap);
    table_init(table);
}

bool table_get(Table* table, Obj_String* key, Value* value) {
    if (table->count == 0)  return false;

    Table_Entry* entry = _entry_find(table->entries, table->cap, key);
    if(entry == NULL) return false;

    *value = entry->value;
    return true;
}

Obj_String* table_find_string(Table* table, const char* chars, int length, uint32_t hash) {
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->cap;
    for (;;) {
        Table_Entry* entry = &table->entries[index];
        if (entry->key == NULL) {
            // Stop if we find an empty non-tonbstone entry.
            if (IS_NIL(entry->value)) return NULL;
        } else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0) {
            // We found it
            return entry->key;
        }

        index = (index + 1) % table->cap;
    }

}

bool table_set(Table* table, Obj_String* key, Value value) {
    if (table->count + 1 > table->cap * TABLE_MAX_LOAD) {
        int cap = GROW_CAPACITY(table->cap);
        _table_adjust_cap(table, cap);
    }

    Table_Entry* entry = _entry_find(table->entries, table->cap, key);
    bool is_new_key    = entry->key == NULL;
    if (is_new_key && IS_NIL(entry->value)) table->count += 1;

    entry->key   = key;
    entry->value = value;
    return is_new_key;
}

bool table_delete(Table* table, Obj_String* key) {
    if (table->cap == 0) return false;

    // Find the entry.
    Table_Entry* entry = _entry_find(table->entries, table->cap, key);
    if (entry == NULL) return false;

    // Place a tombstone in the entry.
    entry->key   = NULL;
    entry->value = V_BOOL(true);
    return true;

}

void table_copy(Table* from, Table* to) {
    for (int i = 0; i < from->cap; i += 1) {
        Table_Entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            table_set(to, entry->key, entry->value);

        }
    }
}

static Table_Entry* _entry_find(Table_Entry* entries, int cap, Obj_String* key) {
    uint32_t index         = key->hash % cap;
    Table_Entry* tombstone = NULL;

    for (;;) {
        Table_Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) {
                // Empty entry
                return tombstone != NULL ? tombstone : entry;
            } else {
                // We found a tombstone.
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            // We found the key
            return entry;
        }

        index = (index + 1) % cap;
    }
}

static void _table_adjust_cap(Table* table, int cap) {
    Table_Entry* entries = ALLOCATE(Table_Entry, cap);

    for (int i = 0; i < cap; i += 1) {
        entries[i].key = NULL;
        entries[i].value = V_NIL;
    }

    table->count = 0;
    for (int i = 0; i < cap; i += 1) {
        Table_Entry* entry = &entries[i];
        if (entry->key == NULL) continue;

        Table_Entry* dest = _entry_find(entries, cap, entry->key);
        dest->key     = entry->key;
        dest->value   = entry->value;
        table->count += 1;
    }

    FREE_ARRAY(Table_Entry, table->entries, table->cap);
    table->entries = entries;
    table->cap     = cap;
}

void mark_table(Table* table) {
    for (int i = 0; i < table->cap; i += 1) {
        Table_Entry* entry = &table->entries[i];
        mark_object((Obj*) entry->key);
        mark_value(entry->value);
    }
}
