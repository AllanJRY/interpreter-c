#ifndef INTERP_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define AS_FUNCTION(value) ((Obj_Function*) AS_OBJ(value))
#define AS_STRING(value) ((Obj_String*) AS_OBJ(value))
#define AS_CSTRING(value) (((Obj_String*) AS_OBJ(value))->chars)

typedef enum Obj_Type {
    OBJ_FUNCTION,
    OBJ_STRING,
} Obj_Type;

struct Obj {
    Obj_Type type;
    Obj*     next;
};

typedef struct Obj_Function {
    Obj         obj;
    int         arity;
    Chunk       chunk;
    Obj_String* name;
} Obj_Function;

struct Obj_String {
    Obj      obj;
    int      length;
    char*    chars;
    uint32_t hash;
};

Obj_String* string_copy(const char* chars, int length);
Obj_String* string_take(char* chars, int length);

Obj_Function* function_new(void);

void object_print(Value value);

static inline bool is_obj_type(Value value, Obj_Type type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define INTERP_OBJECT_H
#endif
