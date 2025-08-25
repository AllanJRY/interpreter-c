#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

#define _ALLOCATE_OBJ(type, obj_type) (type*) _object_allocate(sizeof(type), obj_type)

static void _function_print(Obj_Function* function);

static Obj* _object_allocate(size_t size, Obj_Type type) {
    Obj* object       = (Obj*) reallocate(NULL, 0, size);
    object->type      = type;
    object->is_marked = false;
    object->next      = vm.objects;
    vm.objects        = object;

    #ifdef DEBUG_LOG_GC
        printf("%p allocate %zu for %d\n", (void*) object, size, type);
    #endif

    return object;
}

static Obj_String* _string_allocate(char* chars, int length, uint32_t hash) {
    Obj_String* string = _ALLOCATE_OBJ(Obj_String, OBJ_STRING);
    string->length     = length;
    string->chars      = chars;
    string->hash       = hash;
    vm_stack_push(V_OBJ(string));
    table_set(&vm.strings, string, V_NIL);
    vm_stack_pop();
    return string;
}

static uint32_t _string_hash(const char* key, int length) {
    // FNV-1a hash function.
    uint32_t hash = 2166136261u;
    for(int i = 0; i < length; i += 1) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

Obj_String* string_copy(const char* chars, int length) {
    uint32_t hash = _string_hash(chars, length);

    Obj_String* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return _string_allocate(heap_chars, length, hash);
}

Obj_String* string_take(char* chars, int length) {
    uint32_t hash = _string_hash(chars, length);

    Obj_String* interned = table_find_string(&vm.strings, chars, length, hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return _string_allocate(chars, length, hash);
}

Obj_Upvalue* upvalue_new(Value* slot) {
    Obj_Upvalue* upvalue = _ALLOCATE_OBJ(Obj_Upvalue, OBJ_UPVALUE);
    upvalue->location    = slot;
    upvalue->closed      = V_NIL;
    upvalue->next        = NULL;
    return upvalue;
}

Obj_Function* function_new(void) {
    Obj_Function* function  = _ALLOCATE_OBJ(Obj_Function, OBJ_FUNCTION);
    function->arity         = 0;
    function->upvalue_count = 0;
    function->name          = NULL;
    chunk_init(&function->chunk);
    return function;
}

Obj_Closure* closure_new(Obj_Function* function) {
    Obj_Upvalue** upvalues = ALLOCATE(Obj_Upvalue*, function->upvalue_count);

    for(int i = 0; i < function->upvalue_count; i += 1) {
        upvalues[i] = NULL;
    }

    Obj_Closure* closure   = _ALLOCATE_OBJ(Obj_Closure, OBJ_CLOSURE);
    closure->function      = function;
    closure->upvalues      = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

Obj_Native* native_new(Native_Fn function) {
    Obj_Native* native = _ALLOCATE_OBJ(Obj_Native, OBJ_NATIVE);
    native->function   = function;
    return native;
}

Obj_Class* class_new(Obj_String* name) {
    Obj_Class* new_class = _ALLOCATE_OBJ(Obj_Class, OBJ_CLASS);
    new_class->name     = name;
    return new_class;
}

Obj_Instance* instance_new(Obj_Class* class) {
    Obj_Instance* instance = _ALLOCATE_OBJ(Obj_Instance, OBJ_INSTANCE);
    instance->class        = class;
    table_init(&instance->fields);
    return instance;
}

void object_print(Value value) {
    switch(OBJ_TYPE(value)) {
        case OBJ_CLASS: {
            printf("%s", AS_CLASS(value)->name->chars);
            break;
        }
        case OBJ_CLOSURE: {
            _function_print(AS_CLOSURE(value)->function);
            break;
        }
        case OBJ_FUNCTION: {
            _function_print(AS_FUNCTION(value));
            break;
        }
        case OBJ_INSTANCE: {
            printf("%s instance", AS_INSTANCE(value)->class->name->chars);
            break;
        }
        case OBJ_NATIVE: {
            printf("<native fn>");
            break;
        }
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_UPVALUE: {
            printf("upvalue");
            break;
        }
    }
}

static void _function_print(Obj_Function* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}
