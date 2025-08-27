#ifndef INTERP_OBJECT_H

#include "common.h"
#include "chunk.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_BOUND_METHOD(value) is_obj_type(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value) is_obj_type(value, OBJ_CLASS)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_BOUND_METHOD(value) ((Obj_Bound_Method*) AS_OBJ(value))
#define AS_CLASS(value) ((Obj_Class*) AS_OBJ(value))
#define AS_CLOSURE(value) ((Obj_Closure*) AS_OBJ(value))
#define AS_FUNCTION(value) ((Obj_Function*) AS_OBJ(value))
#define AS_INSTANCE(value) ((Obj_Instance*) AS_OBJ(value))
#define AS_NATIVE(value) (((Obj_Native*) AS_OBJ(value))->function)
#define AS_STRING(value) ((Obj_String*) AS_OBJ(value))
#define AS_CSTRING(value) (((Obj_String*) AS_OBJ(value))->chars)

typedef enum Obj_Type {
    OBJ_BOUND_METHOD,
    OBJ_CLASS,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
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

typedef struct Obj_Class {
    Obj         obj;
    Obj_String* name;
    Table       methods;
} Obj_Class;

typedef struct Obj_Instance {
    Obj        obj;
    Obj_Class* class;
    Table      fields;
} Obj_Instance;

typedef struct Obj_Bound_Method {
    Obj          obj;
    Value        receiver;
    Obj_Closure* method;
} Obj_Bound_Method;

Obj_String* string_copy(const char* chars, int length);
Obj_String* string_take(char* chars, int length);

Obj_Upvalue* upvalue_new(Value* slot);

Obj_Function* function_new(void);

Obj_Closure* closure_new(Obj_Function* function);

Obj_Native* native_new(Native_Fn function);

Obj_Class* class_new(Obj_String* name);

Obj_Instance* instance_new(Obj_Class* class);

Obj_Bound_Method* bound_method_new(Value receiver, Obj_Closure* method);

void object_print(Value value);

static inline bool is_obj_type(Value value, Obj_Type type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define INTERP_OBJECT_H
#endif
