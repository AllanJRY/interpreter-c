#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void chunk_init(Chunk* chunk) {
    chunk->cap   = 0;
    chunk->len   = 0;
    chunk->code  = NULL;
    chunk->lines = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
    FREE_ARRAY(int, chunk->lines, chunk->cap);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_write(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->cap < chunk->len + 1) {
        int old_cap = chunk->cap;
        chunk->cap   = GROW_CAPACITY(old_cap);
        chunk->code  = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->cap);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_cap, chunk->cap);
    }

    chunk->code[chunk->len]  = byte;
    chunk->lines[chunk->len] = line;
    chunk->len              += 1;
}

int chunk_constants_add(Chunk* chunk, Value value){
    value_array_write(&chunk->constants, value);
    return chunk->constants.len - 1;
}
