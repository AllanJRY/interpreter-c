#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

static void _vm_runtime_error(const char* format, ...);

static Value _vm_stack_peek(int distance);
static void  _vm_stack_reset(void);

VM vm;

void vm_init(void) {
    _vm_stack_reset();
}

void vm_free(void) { }

static Interpret_Result _vm_run(void) {
    // NOTE(AJA): Post increment is important here, because we return the current instruction pointer address,
    //            and then, and only then, we increment the instruction pointer address.
    #define READ_BYTE() (*vm.ip++)  
    #define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])  
    #define BINARY_OP(value_type, op)                                          \
    do {                                                                       \
        if (!IS_NUMBER(_vm_stack_peek(0)) || !IS_NUMBER(_vm_stack_peek(1))) { \
            _vm_runtime_error("Operands must be numbers.");                    \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(vm_stack_pop());                                  \
        double a = AS_NUMBER(vm_stack_pop());                                  \
        vm_stack_push(value_type(a op b));                                     \
    } while (false)                                                            \

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
            case OP_NIL: {
                vm_stack_push(V_NIL);
                break;
            }
            case OP_TRUE: {
                vm_stack_push(V_BOOL(true));
                break;
            }
            case OP_FALSE: {
                vm_stack_push(V_BOOL(false));
                break;
            }
            case OP_ADD: {
                BINARY_OP(V_NUMBER, +);
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(V_NUMBER, -);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(V_NUMBER, *);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(V_NUMBER, /);
                break;
            }
            case OP_NEGATE: {
                if (!IS_NUMBER(_vm_stack_peek(0))) {
                    _vm_runtime_error("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                *(vm.stack_top - 1) = V_NUMBER(-AS_NUMBER(*(vm.stack_top - 1)));
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
    Chunk chunk;
    chunk_init(&chunk);

    if (!compiler_compile(source, &chunk)) {
        chunk_free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip    = vm.chunk->code;

    Interpret_Result result = _vm_run();

    chunk_free(&chunk);
    return result;
}

void vm_stack_push(Value value) {
    *vm.stack_top = value;
    vm.stack_top += 1;
}

Value vm_stack_pop(void) {
    vm.stack_top -= 1;
    return *vm.stack_top;
}

static Value _vm_stack_peek(int distance) {
    return vm.stack_top[-1 - distance];
}

static void _vm_stack_reset(void) {
    vm.stack_top = vm.stack;
}

static void _vm_runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    _vm_stack_reset();
}
