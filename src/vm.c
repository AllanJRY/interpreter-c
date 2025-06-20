#include <stdio.h>

#include "common.h"
#include "compiler.h"
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
    #define BINARY_OP(op)          \
    do {                           \
        double b = vm_stack_pop(); \
        double a = vm_stack_pop(); \
        vm_stack_push(a op b);     \
    } while (false)                \

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
            case OP_ADD: {
                BINARY_OP(+);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }
            case OP_NEGATE: {
                *(vm.stack_top - 1) = - *(vm.stack_top - 1);
                // previously (the solution above avoid modifying the stack_top ptr unnecessarily) :
                // vm_stack_push( - vm_stack_pop());
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
    #undef BINARY_OP
}

Interpret_Result vm_interpret(const char* source) {
    compiler_compile(source);
    return INTERPRET_OK;
}

void vm_stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top += 1;
}

Value vm_stack_pop(void) {
    vm.stack_top -= 1;
    return *vm.stack_top;
}
