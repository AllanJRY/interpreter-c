#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

#define _ALLOCATE_OBJ(type, obj_type) (type*) _object_allocate(sizeof(type), obj_type)

static Obj* _object_allocate(size_t size, Obj_Type type) {
    Obj* object = (Obj*) reallocate(NULL, 0, size);
    object->type = type;
    return object;
}

static Obj_String* _string_allocate(char* chars, int length) {
    Obj_String* string = _ALLOCATE_OBJ(Obj_String, OBJ_STRING);
    string->length     = length;
    string->chars      = chars;
    return string;
}

Obj_String* string_copy(const char* chars, int length) {
    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return _string_allocate(heap_chars, length);
}

Obj_String* string_take(char* chars, int length) {
    return _string_allocate(chars, length);

}

void object_print(Value value) {
    switch(OBJ_TYPE(value)) {
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
    }
}
