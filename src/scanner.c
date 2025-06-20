#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct Scanner {
    const char* start;
    const char* current;
    int         line;
} Scanner;

Scanner scanner;

void scanner_init(const char* source) {
    scanner.start   = source;
    scanner.current = source;
    scanner.line    = 1;
}
