#ifndef INTERP_VALUE_H

#include "common.h"

typedef struct Obj Obj;
typedef struct Obj_String Obj_String;

typedef enum Value_Type {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ,
} Value_Type;

typedef struct Value {
    Value_Type type;
    union {
        bool   boolean;
        double number;
        Obj*   obj;
    } as;
} Value;

bool value_equal(Value a, Value b);

#define V_BOOL(value) ((Value) { VAL_BOOL, {.boolean = value} })
#define V_NIL ((Value) { VAL_NIL, {.number = 0} })
#define V_NUMBER(value) ((Value) { VAL_NUMBER, {.number = value} })
#define V_OBJ(object) ((Value) { VAL_OBJ, {.obj = (Obj*)object} })

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_OBJ(value) ((value).as.obj)
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
