#ifndef INTERP_COMPILER_H

#include "vm.h"

bool compiler_compile(const char* source, Chunk* chunk);

#define INTERP_COMPILER_H
#endif
