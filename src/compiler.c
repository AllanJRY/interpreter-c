#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compiler_compile(const char* source) {
    scanner_init(source);
}
