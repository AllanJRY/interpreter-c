#ifndef INTERP_VM_H

#include "chunk.h"

typedef struct VM {
    Chunk*   chunk;
    uint8_t* ip; // Instruction pointer.
} VM;

typedef enum Interpret_Result {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret(Chunk* chunk);

#define INTERP_VM_H
#endif
