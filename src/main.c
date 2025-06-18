#include "common.h"
#include "chunk.h"
#include "debug.h"

#include "memory.c"
#include "chunk.c"
#include "debug.c"

int main (int argc, const char* argv[]) {
    (void) argc;
    (void) argv;

    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN);
    disassemble_chunk(&chunk, "Test chunk");
    free_chunk(&chunk);
    return 0;
}
