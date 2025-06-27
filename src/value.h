#ifndef INTERP_VALUE_H

#include "common.h"

typedef enum Value_Type {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
} Value_Type;

typedef struct Value {
    Value_Type type;
    union {
        bool   boolean;
        double number;
    } as;
} Value;

#define V_BOOL(value) ((Value) { VAL_BOOL, {.boolean = value} })
#define V_NIL(value) ((Value) { VAL_NIL, {.number = 0} })
#define V_NUMBER(value) ((Value) { VAL_NUMBER, {.number = value} })

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

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
