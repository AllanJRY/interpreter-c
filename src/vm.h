#ifndef INTERP_VM_H

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct VM {
    Chunk*   chunk;
    uint8_t* ip; // Instruction pointer.l
    Value    stack[STACK_MAX];
    Value*   stack_top;
} VM;

typedef enum Interpret_Result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret(const char* source);
void vm_stack_push(Value value);
Value vm_stack_pop(void);

#define INTERP_VM_H
#endif
