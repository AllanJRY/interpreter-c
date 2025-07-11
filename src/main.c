#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

#include "vm.c"
#include "memory.c"
#include "value.c"
#include "table.c"
#include "object.c"
#include "chunk.c"
#include "scanner.c"
#include "compiler.c"
#include "debug.c"

static void  _repl(void);
static void  _file_run(const char* path);
static char* _file_read(const char* path);

int main (int argc, const char* argv[]) {
    vm_init();

    if (argc == 1) {
        _repl();
    } else if (argc == 2) {
        _file_run(argv[1]);
    } else {
        fprintf(stderr, "Usage: interp [path]\n");
        exit(64);
    }

    vm_free();
    return 0;
}

static void _repl(void) {
    char line[1024];
    for (;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        vm_interpret(line);
    }
}

static void _file_run(const char* path) {
    char* source = _file_read(path);
    Interpret_Result result = vm_interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

static char* _file_read(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*) malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytes_read  = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes_read] = '\0';

    fclose(file);

    return buffer;
}
