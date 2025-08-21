#ifndef INTERP_COMPILER_H

#include "object.h"
#include "vm.h"

Obj_Function* compiler_compile(const char* source);

void mark_compiler_roots(void);

#define INTERP_COMPILER_H
#endif
