#include <stdio.h>
#include <string.h>

#include "object.h"
#include "memory.h"
#include "value.h"

bool value_equal(Value a, Value b) {
    if (a.type != b.type) return false;

    switch(a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
        default: return false; // Unreachable.
    }
}

void value_array_init(Value_Array* array) {
    array->cap    = 0;
    array->len    = 0;
    array->values = NULL;
}

void value_array_write(Value_Array* array, Value value) {
    if (array->cap < array->len + 1) {
        int old_cap   = array->cap;
        array->cap    = GROW_CAPACITY(old_cap);
        array->values = GROW_ARRAY(Value, array->values, old_cap, array->cap);
    }

    array->values[array->len] = value;
    array->len               += 1;
}

void value_array_free(Value_Array* array) {
    FREE_ARRAY(Value, array->values, array->cap);
    value_array_init(array);
}

void value_print(Value value) {
    switch (value.type) {
        case VAL_BOOL: {
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        }
        case VAL_NIL: {
            printf("nil");
            break;
        }
        case VAL_NUMBER: {
            printf("%g", AS_NUMBER(value));
            break;
        }
        case VAL_OBJ: {
            object_print(value);
            break;
        }
    }
}
