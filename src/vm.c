#include <stdio.h>

#include "common.h"
#include "debug.h"
#include "vm.h"

VM vm;

static void _vm_stack_reset(void) {
    vm.stack_top = vm.stack;
}

void vm_init(void) {
    _vm_stack_reset();
}

void vm_free(void) { }

static Interpret_Result _vm_run(void) {
    // NOTE(AJA): Post increment is important here, because we return the current instruction pointer address,
    //            and then, and only then, we increment the instruction pointer address.
    #define READ_BYTE() (*vm.ip++)  
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])  

    for(;;) {
        #ifdef DEBUG_TRACE_EXECUTION
        printf(" ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot += 1) {
            printf("[ ");
            value_print(*slot);
            printf(" ]");
        }
        printf("\n");
        instruction_disassemble(vm.chunk, (int)(vm.ip - vm.chunk->code));
        #endif

        uint8_t instruction;
        switch(instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vm_stack_push(constant);
                break;
            }
            case OP_RETURN: {
                value_print(vm_stack_pop());
                printf("\n");
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

void vm_stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top += 1;
}

Value vm_stack_pop(void) {
    vm.stack_top -= 1;
    return *vm.stack_top;
}
