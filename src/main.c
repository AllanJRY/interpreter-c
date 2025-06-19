#include "common.h"
#include "chunk.h"
#include "debug.h"

#include "memory.c"
#include "value.c"
#include "chunk.c"
#include "debug.c"

int main (int argc, const char* argv[]) {
    (void) argc;
    (void) argv;

    Chunk chunk;
    chunk_init(&chunk);

    int constant_idx = chunk_constants_add(&chunk, 1.2);
    chunk_write(&chunk, OP_CONSTANT, 123);
    chunk_write(&chunk, constant_idx, 123);

    chunk_write(&chunk, OP_RETURN, 123);

    chunk_disassemble(&chunk, "Test chunk");

    chunk_free(&chunk);
    return 0;
}
