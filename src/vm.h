#ifndef INTERP_VM_H

#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX  (FRAMES_MAX * UINT8_COUNT)

typedef struct Call_Frame {
    Obj_Function* function;
    uint8_t*      ip;
    Value*        slots;
} Call_Frame;

typedef struct VM {
    Call_Frame frames[FRAMES_MAX];
    int        frame_count;
    Value      stack[STACK_MAX];
    Value*     stack_top;
    Table      globals;
    Table      strings;
    Obj*       objects;
} VM;

typedef enum Interpret_Result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

// extern used here to access `vm` from other files (compared to the book, I use unit build, so this should not be necessary, but I want to keep this sync with the book).
extern VM vm;

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret(const char* source);
void vm_stack_push(Value value);
Value vm_stack_pop(void);

#define INTERP_VM_H
#endif
