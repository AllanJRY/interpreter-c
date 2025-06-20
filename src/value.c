#include <stdio.h>

#include "memory.h"
#include "value.h"

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
    printf("%g", value);
}
