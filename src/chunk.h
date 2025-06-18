#ifndef INTERP_CHUNK_H

#include "common.h"

typedef enum Op_Code {
    OP_RETURN,
} OpCode;

typedef struct Chunk {
    int      len;
    int      cap;
    uint8_t* code;
} Chunk;

void init_chunk(Chunk* chunk);
void free_chunk(Chunk* chunk);
void write_chunk(Chunk* chunk, uint8_t byte);

#define INTERP_CHUNK_H
#endif
