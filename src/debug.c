#include <stdio.h>

#include "debug.h"
#include "value.h"


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

int instruction_disassemble(Chunk* chunk, int offset) {
    printf("%04d ", offset);

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_CONSTANT: {
            return instruction_constant("OP_CONSTANT", chunk, offset);
        }
        case OP_RETURN: {
            return instruction_simple("OP_RETURN", offset);
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
