#ifndef INTERP_VALUE_H

#include "common.h"

typedef double Value;

typedef struct Value_Array {
    int    cap;
    int    len;
    Value* values;
} Value_Array;

void value_array_init(Value_Array* array);
void value_array_write(Value_Array* array, Value value);
void value_array_free(Value_Array* array);

void value_print(Value value);

#define INTERP_VALUE_H
#endif
