#ifndef INTERP_VALUE_H

#include <string.h>

#include "common.h"

typedef struct Obj Obj;
typedef struct Obj_String Obj_String;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t) 0x8000000000000000)
#define QNAN     ((uint64_t) 0x7ffc000000000000)

#define TAG_NIL   1 // 01
#define TAG_FALSE 2 // 10
#define TAG_TRUE  3 // 11

typedef uint64_t Value;

#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define AS_NUMBER(value) value_to_num(value)
static inline double value_to_num(Value value) {
    double num;
    memcpy(&num, &value, sizeof(Value));
    return num;
}
#define V_NUMBER(num) num_to_value(num)
static inline Value num_to_value(double num) {
    Value value;
    memcpy(&value, &num, sizeof(double));
    return value;
}

#define IS_NIL(value)  ((value) == V_NIL)
#define V_NIL ((Value) (uint64_t) (QNAN | TAG_NIL))

#define IS_BOOL(value) (((value) | 1) == V_TRUE)
#define AS_BOOL(value) ((value) == V_TRUE)
#define V_BOOL(b)      ((b) ? V_TRUE : V_FALSE)
#define V_FALSE        ((Value) (uint64_t) (QNAN | TAG_FALSE))
#define V_TRUE         ((Value) (uint64_t) (QNAN | TAG_TRUE))

#define IS_OBJ(value) (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#define AS_OBJ(value) ((Obj*) (uintptr_t) ((value) & ~(SIGN_BIT | QNAN)))
#define V_OBJ(obj)    (Value) (SIGN_BIT | QNAN | (uint64_t)(uintptr_t) (obj))

#else

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

#endif

bool value_equal(Value a, Value b);

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
