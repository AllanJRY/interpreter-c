#include <stdio.h>

#include "debug.h"
#include "value.h"

static int _instruction_byte(const char* name, Chunk* chunk, int offset);
static int _instruction_jump(const char* name, int sign, Chunk* chunk, int offset);

static int instruction_simple(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int instruction_constant(const char* name, Chunk* chunk, int offset) {
    uint8_t constant_idx = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant_idx);
    value_print(chunk->constants.values[constant_idx]);
    printf("'\n");
    return offset + 2;
}

static int _instruction_byte(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int _instruction_jump(const char* name, int sign, Chunk* chunk, int offset){
    uint16_t jump = (uint16_t) (chunk->code[offset + 1] << 8);
    jump         |= chunk->code[offset + 2];
    printf("%-16s %4d ->%d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int instruction_disassemble(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT: {
            return instruction_constant("OP_CONSTANT", chunk, offset);
        }
        case OP_NIL: {
            return instruction_simple("OP_NIL", offset);
        }
        case OP_TRUE: {
            return instruction_simple("OP_TRUE", offset);
        }
        case OP_FALSE: {
            return instruction_simple("OP_FALSE", offset);
        }
        case OP_POP: {
            return instruction_simple("OP_POP", offset);
        }
        case OP_GET_LOCAL: {
            return _instruction_byte("OP_GET_LOCAL", chunk, offset);
        }
        case OP_GET_GLOBAL: {
            return instruction_constant("OP_GET_GLOBAL", chunk, offset);
        }
        case OP_DEFINE_GLOBAL: {
            return instruction_constant("OP_DEFINE_GLOBAL", chunk, offset);
        }
        case OP_SET_LOCAL: {
            return _instruction_byte("OP_SET_LOCAL", chunk, offset);
        }
        case OP_SET_GLOBAL: {
            return instruction_constant("OP_SET_GLOBAL", chunk, offset);
        }
        case OP_EQUAL: {
            return instruction_simple("OP_EQUAL", offset);
        }
        case OP_GREATER: {
            return instruction_simple("OP_GREATER", offset);
        }
        case OP_LESS: {
            return instruction_simple("OP_LESS", offset);
        }
        case OP_ADD: {
            return instruction_simple("OP_ADD", offset);
        }
        case OP_SUBTRACT: {
            return instruction_simple("OP_SUBTRACT", offset);
        }
        case OP_MULTIPLY: {
            return instruction_simple("OP_MULTIPLY", offset);
        }
        case OP_DIVIDE: {
            return instruction_simple("OP_DIVIDE", offset);
        }
        case OP_NOT: {
            return instruction_simple("OP_NOT", offset);
        }
        case OP_NEGATE: {
            return instruction_simple("OP_NEGATE", offset);
        }
        case OP_PRINT: {
            return instruction_simple("OP_PRINT", offset);
        }
        case OP_JUMP: {
            return _instruction_jump("OP_JUMP", 1, chunk, offset);
        }
        case OP_JUMP_IF_FALSE: {
            return _instruction_jump("OP_JUMP_IF_FALSE", 1, chunk, offset);
        }
        case OP_LOOP: {
            return _instruction_jump("OP_LOOP", -1, chunk, offset);
        }
        case OP_RETURN: {
            return instruction_simple("OP_RETURN", offset);
        }
        case OP_CALL: {
            return _instruction_byte("OP_CALL", chunk, offset);
        }
        default: {
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
        }
    }

}

void chunk_disassemble(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->len;) {
        offset = instruction_disassemble(chunk, offset);
    }
}
