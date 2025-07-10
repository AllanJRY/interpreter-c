#include <stdlib.h>

#include "memory.h"
#include "vm.h"

static void _free_object(Obj* object);

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    (void) old_size;

    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(1);
    return result;
}

void mem_free_objects(void) {
    Obj* object = vm.objects;

    while(object != NULL) {
        Obj* next = object->next;
        _free_object(object);
        object = next;
    }
}

static void _free_object(Obj* object) {
    switch(object->type) {
        case OBJ_STRING: {
            Obj_String* string = (Obj_String*) object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(Obj_String, object);
            break;
        }
    }
}
