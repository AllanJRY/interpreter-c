#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

static void _mark_roots(void);
static void _trace_references(void);
static void _sweep(void);
static void _mark_array(Value_Array* array);

static void _free_object(Obj* object);
static void _blacken_object(Obj* object);

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
    vm.bytes_allocated += new_size - old_size;

    if (new_size > old_size) {
        #ifdef DEBUG_STRESS_GC
            collect_garbage();
        #endif
        if (vm.bytes_allocated > vm.next_gc) {
            collect_garbage();
        }
    }

    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, new_size);
    if (result == NULL) exit(1);
    return result;
}

void collect_garbage(void) {
    #ifdef DEBUG_LOG_GC
        printf("-- gc begin\n");
        size_t before = vm.bytes_allocated;
    #endif

    _mark_roots();
    _trace_references();
    table_remove_white(&vm.strings);
    _sweep();
    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

    #ifdef DEBUG_LOG_GC
        printf("-- gc end\n");
        printf(" collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
    #endif
}

static void _mark_roots(void) {
    for (Value* slot = vm.stack; slot < vm.stack_top; slot += 1) {
        mark_value(*slot);
    }

    for (int i = 0; i < vm.frame_count; i += 1) {
        mark_object((Obj*) vm.frames[i].closure);
    }

    for (Obj_Upvalue* upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        mark_object((Obj*) upvalue);
    }

    mark_table(&vm.globals);
    mark_compiler_roots();
}

static void _trace_references(void) {
    while(vm.gray_count > 0) {
        Obj* object = vm.gray_stack[--vm.gray_count];
        _blacken_object(object);
    }
}

static void _blacken_object(Obj* object) {
    #ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*) object);
        value_print(V_OBJ(object));
        printf("\n");
    #endif

    switch(object->type) {
        case OBJ_UPVALUE: {
            mark_value(((Obj_Upvalue*) object)->closed);
            break;
        }
        case OBJ_FUNCTION: {
            Obj_Function* function = (Obj_Function*) object;
            mark_object((Obj*) function->name);
            _mark_array(&function->chunk.constants);
            break;
        }
        case OBJ_CLOSURE: {
            Obj_Closure* closure = (Obj_Closure*) object;
            mark_object((Obj*) closure->function);
            for (int i = 0; i < closure->upvalue_count; i += 1) {
                mark_object((Obj*) closure->upvalues[i]);
            }
            break;
        }
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

static void _sweep(void) {
    Obj* previous = NULL;
    Obj* object   = vm.objects;

    while(object != NULL) {
        if (object->is_marked) {
            object->is_marked = false;
            previous          = object;
            object            = object->next;
        } else {
            Obj* unreached = object;
            object         = object->next;

            if(previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            _free_object(unreached);
        }
    }
}

void mark_value(Value value) {
    if (IS_OBJ(value)) mark_object(AS_OBJ(value));
}

void mark_object(Obj* object) {
    if (object == NULL) return;
    if (object->is_marked) return;

    #ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*) object);
        value_print(V_OBJ(object));
        printf("\n");
    #endif

    object->is_marked = true;

    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        vm.gray_stack = (Obj**) realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
        if (vm.gray_stack == NULL) exit(1);
    }

    vm.gray_stack[vm.gray_count++] = object;
}

static void _mark_array(Value_Array* array) {
    for (int i = 0; i < array->len; i += 1) {
        mark_value(array->values[i]);
    }
}

void mem_free_objects(void) {
    Obj* object = vm.objects;

    while(object != NULL) {
        Obj* next = object->next;
        _free_object(object);
        object = next;
    }

    free(vm.gray_stack);
}

static void _free_object(Obj* object) {
    #ifdef DEBUG_LOG_GC
        printf("%p free type %d\n", (void*) object, object->type);
    #endif

    switch(object->type) {
        case OBJ_CLOSURE: {
            Obj_Closure* closure = (Obj_Closure*)object;
            FREE_ARRAY(Obj_Upvalue*, closure->upvalues, closure->upvalue_count);
            FREE(Obj_Closure, object);
            break;
        }
        case OBJ_FUNCTION: {
            Obj_Function* function = (Obj_Function*) object;
            chunk_free(&function->chunk);
            FREE(Obj_Function, object);
            break;
        }
        case OBJ_NATIVE: {
            FREE(Obj_Native, object);
            break;
        }
        case OBJ_STRING: {
            Obj_String* string = (Obj_String*) object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(Obj_String, object);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(Obj_Upvalue, object);
            break;
        }
    }
}
