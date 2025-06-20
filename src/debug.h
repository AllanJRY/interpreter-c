#ifndef INTERP_DEBUG_H

#include "chunk.h"

void chunk_disassemble(Chunk* chunk, const char* name);
int instruction_disassemble(Chunk* chunk, int offset);

#define INTERP_DEBUG_H
#endif
