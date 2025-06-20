#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

static bool _scanner_is_at_end(void);
static char _scanner_advance(void);
static bool _scanner_advance_if_match(char expected);
static char _scanner_peek(void);
static void _scanner_skip_whitespace(void);

static Scanner_Token _token_make(Scanner_Token_Type type);
static Scanner_Token _token_error(const char* msg);

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

Scanner_Token scanner_scan_token(void) {
    _scanner_skip_whitespace();
    scanner.start = scanner.current;

    if (_scanner_is_at_end()) return _token_make(TOKEN_EOF);
    char c = _scanner_advance();

    switch(c) {
        case '(': return _token_make(TOKEN_LEFT_PAREN);
        case '(': return _token_make(TOKEN_RIGHT_PAREN);
        case '{': return _token_make(TOKEN_LEFT_BRACE);
        case '}': return _token_make(TOKEN_RIGHT_BRACE);
        case ';': return _token_make(TOKEN_SEMICOLON);
        case ',': return _token_make(TOKEN_COMMA);
        case '.': return _token_make(TOKEN_DOT);
        case '-': return _token_make(TOKEN_MINUS);
        case '+': return _token_make(TOKEN_PLUS);
        case '/': return _token_make(TOKEN_SLASH);
        case '*': return _token_make(TOKEN_STAR);
        case '!': return _token_make(_scanner_advance_if_match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return _token_make(_scanner_advance_if_match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return _token_make(_scanner_advance_if_match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>': return _token_make(_scanner_advance_if_match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    }

    return _token_error("Unexpected character.");
}

static bool _scanner_is_at_end(void) {
    return *scanner.current == '\0';
}

static char _scanner_advance(void) {
    scanner.current += 1;
    return scanner.current[-1];
}

static bool _scanner_advance_if_match(char expected) {
    if (_scanner_is_at_end()) return false;
    if (*scanner.current != expected) return false;
    scanner.current += 1;
    return true;
}

static char _scanner_peek(void) {
    return *scanner.current;
}

static void _scanner_skip_whitespace(void) {
    for(;;) {
        char c = _scanner_peek();
        switch c {
            case ' ':
            case '\r':
            case '\t': {
                _scanner_advance();
                break;
            }
            case '\n': {
                scanner.line += 1;
                _scanner_advance();
                break;
            }
            default: return;
        }
    }
}

static Scanner_Token _token_make(Scanner_Token_Type type) {
    Scanner_Token token;
    token.type   = type;
    token.start  = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    token.line   = scanner.line;
    return token;
}

static Scanner_Token _token_error(const char* msg) {
    Scanner_Token token;
    token.type   = TOKEN_ERROR;
    token.start  = msg;
    token.length = (int) strlen(msg);
    token.line   = scanner.line;
    return token;
}
