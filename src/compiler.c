#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

static void _error(const char* msg);
static void _error_at_current(const char* msg);
static void _error_at(Scanner_Token* token, const char* msg);

static void _parser_advance(void);
static void _parser_consume(Scanner_Token_Type type, const char* msg);

typedef struct Parser {
    Scanner_Token current;
    Scanner_Token previous;
    bool          had_error;
    bool          panic_mode;
} Parser;

Parser parser;

bool compiler_compile(const char* source, Chunk* chunk) {
    scanner_init(source);
    parser.had_error  = false;
    parser.panic_mode = false;

    _parser_advance();
    //_expression();
    _parser_consume(TOKEN_EOF, "Expect end of expression.");
    return !parser.had_error;
}

static void _parser_advance(void) {
    parser.previous = parser.current;

    for(;;) {
        parser.current = scanner_scan_token();
        if (parser.current.type != TOKEN_ERROR) break;

        _error_at_current(parser.current.start);
    }
}

static void _parser_consume(Scanner_Token_Type type, const char* msg) {
    if (parser.current.type == type) {
        _parser_advance();
        return;
    }
    
    _error_at_current(msg);
}

static void _error(const char* msg) {
    _error_at(&parser.previous, msg);
}

static void _error_at_current(const char* msg) {
    _error_at(&parser.current, msg);
}

static void _error_at(Scanner_Token* token, const char* msg) {
    if (parser.panic_mode) return;
    parser.panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if(token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if(token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", msg);
    parser.had_error = true;
}
