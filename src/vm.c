#include <stdio.h>

#include "common.h"
#include "vm.h"

VM vm;

void vm_init(void) { }

void vm_free(void) { }

static Interpret_Result _vm_run(void) {
    // NOTE(AJA): Post increment is important here, because we return the current instruction pointer address,
    //            and then, and only then, we increment the instruction pointer address.
    #define READ_BYTE() (*vm.ip++)  
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])  

    for(;;) {
        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                value_print(constant);
                printf("\n");
                break;
            }
            case OP_RETURN: {
                return INTERPRET_OK;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
}

Interpret_Result vm_interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip    = vm.chunk->code;
    return _vm_run();
}
