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
    Obj* object  = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects   = object;
    return object;
}

static Obj_String* _string_allocate(char* chars, int length, uint32_t hash) {
    Obj_String* string = _ALLOCATE_OBJ(Obj_String, OBJ_STRING);
    string->length     = length;
    string->chars      = chars;
    string->hash       = hash;
    table_set(&vm.strings, string, V_NIL);
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

Obj_Function* function_new(void) {
    Obj_Function* function = _ALLOCATE_OBJ(Obj_Function, OBJ_FUNCTION);
    function->arity        = 0;
    function->name         = NULL;
    chunk_init(&function->chunk);
    return function;
}

Obj_Native* native_new(Native_Fn function) {
    Obj_Native* native = _ALLOCATE_OBJ(Obj_Native, OBJ_NATIVE);
    native->function   = function;
    return native;
}

void object_print(Value value) {
    switch(OBJ_TYPE(value)) {
        case OBJ_FUNCTION: {
            _function_print(AS_FUNCTION(value));
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
    }
}

static void _function_print(Obj_Function* function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }
    printf("<fn %s>", function->name->chars);
}
