#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

static void _vm_runtime_error(const char* format, ...);

static bool _is_falsey(Value value);
static void _concatenate(void);

static Interpret_Result _vm_run(void);

static Value _vm_stack_peek(int distance);
static void  _vm_stack_reset(void);

static bool _call_value(Value callee, int arg_count);
static bool _call(Obj_Function* function, int arg_count);

static Value _native_clock(int arg_count, Value* args);
static void  _native_define(const char* name, Native_Fn function);

VM vm;

void vm_init(void) {
    _vm_stack_reset();
    vm.objects = NULL;
    table_init(&vm.globals);
    table_init(&vm.strings);
    _native_define("clock", _native_clock);
}

void vm_free(void) {
    table_free(&vm.globals);
    table_free(&vm.strings);
    mem_free_objects();
}

Interpret_Result vm_interpret(const char* source) {
    Obj_Function* function = compiler_compile(source);
    if(function == NULL) return INTERPRET_COMPILE_ERROR;

    vm_stack_push(V_OBJ(function));
    _call(function, 0);

    return _vm_run();
}

static Interpret_Result _vm_run(void) {
    Call_Frame* frame = &vm.frames[vm.frame_count - 1];

    // NOTE(AJA): Post increment is important here, because we return the current instruction pointer address,
    //            and then, and only then, we increment the instruction pointer address.
    #define READ_BYTE() (*frame->ip++)  
    #define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])  
    #define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
    #define READ_STRING() (AS_STRING(READ_CONSTANT()))
    #define BINARY_OP(value_type, op)                                          \
    do {                                                                       \
        if (!IS_NUMBER(_vm_stack_peek(0)) || !IS_NUMBER(_vm_stack_peek(1))) {  \
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
        instruction_disassemble(&frame->function->chunk, (int)(frame->ip - frame->function->chunk.code));
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
            case OP_POP: {
                vm_stack_pop();
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm_stack_push(frame->slots[slot]);
                break;
            }
            case OP_GET_GLOBAL: {
                Obj_String* name = READ_STRING();
                Value value;
                if (!table_get(&vm.globals, name, &value)) {
                    _vm_runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                vm_stack_push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                Obj_String* name = READ_STRING();
                table_set(&vm.globals, name, _vm_stack_peek(0));
                vm_stack_pop();
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = _vm_stack_peek(0);
                break;
            }
            case OP_SET_GLOBAL: {
                Obj_String* name = READ_STRING();
                if(table_set(&vm.globals, name, _vm_stack_peek(0))) {
                    table_delete(&vm.globals, name);
                    _vm_runtime_error("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value a = vm_stack_pop();
                Value b = vm_stack_pop();
                vm_stack_push(V_BOOL(value_equal(a ,b)));
                break;
            }
            case OP_GREATER: {
                BINARY_OP(V_BOOL, >);
                break;
            }
            case OP_LESS: {
                BINARY_OP(V_BOOL, <);
                break;
            }
            case OP_ADD: {
                if(IS_STRING(_vm_stack_peek(0)) && IS_STRING(_vm_stack_peek(1))) {
                    _concatenate();
                } else if(IS_NUMBER(_vm_stack_peek(0)) && IS_NUMBER(_vm_stack_peek(1))) {
                    double b = AS_NUMBER(vm_stack_pop());
                    double a = AS_NUMBER(vm_stack_pop());
                    vm_stack_push(V_NUMBER(a + b));
                } else {
                    _vm_runtime_error("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }

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
            case OP_NOT: {
                *(vm.stack_top - 1) = V_BOOL(_is_falsey(*(vm.stack_top - 1)));
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
            case OP_PRINT: {
                value_print(vm_stack_pop());
                printf("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset  = READ_SHORT();
                frame->ip       += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if(_is_falsey(_vm_stack_peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset  = READ_SHORT();
                frame->ip       -= offset;
                break;
            }
            case OP_CALL: {
                int arg_count = READ_BYTE();
                if (!_call_value(_vm_stack_peek(arg_count), arg_count)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
            case OP_RETURN: {
                Value result    = vm_stack_pop();
                vm.frame_count -= 1;

                if (vm.frame_count == 0) {
                    vm_stack_pop();
                    return INTERPRET_OK;
                }

                vm.stack_top = frame->slots;
                vm_stack_push(result);
                frame = &vm.frames[vm.frame_count - 1];
                break;
            }
        }
    }

    #undef READ_BYTE
    #undef READ_STRING
    #undef READ_SHORT
    #undef READ_CONSTANT
    #undef BINARY_OP
}

static bool _is_falsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void _concatenate(void) {
    Obj_String* b = AS_STRING(vm_stack_pop());
    Obj_String* a = AS_STRING(vm_stack_pop());

    int length  = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    
    Obj_String* result = string_take(chars, length);
    vm_stack_push(V_OBJ(result));
}

static bool _call_value(Value callee, int arg_count) {
    if (IS_OBJ(callee)) {
        switch(OBJ_TYPE(callee)) {
            case OBJ_FUNCTION: return _call(AS_FUNCTION(callee), arg_count);
            case OBJ_NATIVE: {
                Native_Fn native  = AS_NATIVE(callee);
                Value result      = native(arg_count, vm.stack_top - arg_count);
                vm.stack_top     -= arg_count + 1;
                vm_stack_push(result);
                return true;
            };
            default: break; // Non-callable object type.
        }
    }

    _vm_runtime_error("Can only call functions and classes");
    return false;
}

static bool _call(Obj_Function* function, int arg_count) {
    if (arg_count != function->arity) {
        _vm_runtime_error("Expected %d arguments but got %d.", function->arity, arg_count);
        return false;
    }

    if (vm.frame_count == FRAMES_MAX) {
        _vm_runtime_error("Stack overflow.");
        return false;
    }

    Call_Frame* frame = &vm.frames[vm.frame_count++];
    frame->function   = function;
    frame->ip         = function->chunk.code;
    frame->slots      = vm.stack_top - arg_count - 1;
    return true;
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

static void _native_define(const char* name, Native_Fn function) {
    vm_stack_push(V_OBJ(string_copy(name, (int) strlen(name))));
    vm_stack_push(V_OBJ(native_new(function)));
    table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    vm_stack_pop();
    vm_stack_pop();
}

static Value _native_clock(int arg_count, Value* args) {
    (void) arg_count;
    (void) args;
    return V_NUMBER((double)clock() / CLOCKS_PER_SEC);
}

static void _vm_runtime_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);


    for (int i = vm.frame_count - 1; i >= 0; i -= 1) {
        Call_Frame* frame = &vm.frames[i];
        Obj_Function* function = frame->function;
        size_t instruction = frame->ip - function->chunk.code - 1;

        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }

    _vm_stack_reset();
}
