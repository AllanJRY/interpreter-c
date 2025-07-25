#ifndef INTERP_COMPILER_H

#include "object.h"
#include "vm.h"

Obj_Function* compiler_compile(const char* source);

#define INTERP_COMPILER_H
#endif
