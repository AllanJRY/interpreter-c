#ifndef INTERP_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define AS_CLOSURE(value) ((Obj_Closure*) AS_OBJ(value))
#define AS_FUNCTION(value) ((Obj_Function*) AS_OBJ(value))
#define AS_NATIVE(value) (((Obj_Native*) AS_OBJ(value))->function)
#define AS_STRING(value) ((Obj_String*) AS_OBJ(value))
#define AS_CSTRING(value) (((Obj_String*) AS_OBJ(value))->chars)

typedef enum Obj_Type {
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_UPVALUE,
} Obj_Type;

struct Obj {
    Obj_Type type;
    bool     is_marked;
    Obj*     next;
};

typedef struct Obj_Function {
    Obj         obj;
    int         arity;
    int         upvalue_count;
    Chunk       chunk;
    Obj_String* name;
} Obj_Function;

typedef Value (*Native_Fn)(int arg_count, Value* args);

typedef struct Obj_Native {
    Obj       obj;
    Native_Fn function;
} Obj_Native;

struct Obj_String {
    Obj      obj;
    int      length;
    char*    chars;
    uint32_t hash;
};

typedef struct Obj_Upvalue {
    Obj    obj;
    Value* location;
    Value  closed;
    struct Obj_Upvalue* next;
} Obj_Upvalue;

typedef struct Obj_Closure {
    Obj           obj;
    Obj_Function* function;
    Obj_Upvalue** upvalues;
    int           upvalue_count;
} Obj_Closure;

Obj_String* string_copy(const char* chars, int length);
Obj_String* string_take(char* chars, int length);

Obj_Upvalue* upvalue_new(Value* slot);

Obj_Function* function_new(void);

Obj_Closure* closure_new(Obj_Function* function);

Obj_Native* native_new(Native_Fn function);

void object_print(Value value);

static inline bool is_obj_type(Value value, Obj_Type type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define INTERP_OBJECT_H
#endif
