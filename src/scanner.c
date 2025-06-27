#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

static bool _is_alpha(char c);
static bool _is_digit(char c);
static Scanner_Token_Type _check_keyword(int start, int length, const char* rest, Scanner_Token_Type type);

static bool _scanner_is_at_end(void);
static char _scanner_advance(void);
static bool _scanner_advance_if_match(char expected);
static char _scanner_peek(void);
static char _scanner_peek_next(void);
static void _scanner_skip_whitespace(void);

static Scanner_Token _token_make(Scanner_Token_Type type);
static Scanner_Token _token_make_string(void);
static Scanner_Token _token_make_number(void);
static Scanner_Token _token_make_identifier(void);
static Scanner_Token _token_error(const char* msg);

static Scanner_Token_Type _token_identifier_type(void);

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

    if(_is_alpha(c)) return _token_make_identifier();
    if(_is_digit(c)) return _token_make_number();

    switch(c) {
        case '(': return _token_make(TOKEN_LEFT_PAREN);
        case ')': return _token_make(TOKEN_RIGHT_PAREN);
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
        case '"': return _token_make_string();
        case '\n': return _token_make_string();
    }

    return _token_error("Unexpected character.");
}

static bool _is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool _is_digit(char c) {
    return c >= '0' && c <= '9';
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

static char _scanner_peek_next(void) {
    if (_scanner_is_at_end()) return '\0';
    return *(scanner.current + 1);
}

static void _scanner_skip_whitespace(void) {
    for(;;) {
        char c = _scanner_peek();
        switch (c) {
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
            case '/': {
                if (_scanner_peek_next() == '/') {
                    while(_scanner_peek() != '\n' && !_scanner_is_at_end()) _scanner_advance();
                } else {
                    return;
                }
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

static Scanner_Token _token_make_string(void) {
    while (_scanner_peek() != '"' && !_scanner_is_at_end()) {
        if (_scanner_peek() == '\n') scanner.line += 1;
        _scanner_advance();
    }

    if(_scanner_is_at_end()) return _token_error("Unterminated string");

    // The closing quote.
    _scanner_advance();
    
    return _token_make(TOKEN_STRING);
}

static Scanner_Token _token_make_number(void) {
    while(_is_digit(_scanner_peek())) _scanner_advance();

    // Look for a frational part.
    if (_scanner_peek() == '.' && _is_digit(_scanner_peek_next())) {
        // Comsume the ".".
        _scanner_advance();

        while(_is_digit(_scanner_peek())) _scanner_advance();
    }

    return _token_make(TOKEN_NUMBER);
}

static Scanner_Token _token_make_identifier(void) {
    while (_is_alpha(_scanner_peek()) || _is_digit(_scanner_peek())) _scanner_advance();

    return _token_make(_token_identifier_type());

}

static Scanner_Token_Type _token_identifier_type(void) {
    switch(scanner.start[0]) {
        case 'a': return _check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c': return _check_keyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return _check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f': {
            if (scanner.current - scanner.start > 1) {
                switch(scanner.start[1]) {
                    case 'a': return _check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return _check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return _check_keyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        }
        case 'i': return _check_keyword(1, 1, "f", TOKEN_IF);
        case 'n': return _check_keyword(1, 2, "il", TOKEN_NIL);
        case 'o': return _check_keyword(1, 1, "r", TOKEN_OR);
        case 'p': return _check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return _check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return _check_keyword(1, 4, "uper", TOKEN_SUPER);
        case 't': {
            if (scanner.current - scanner.start > 1) {
                switch(scanner.start[1]) {
                    case 'h': return _check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return _check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        }
        case 'v': return _check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return _check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Scanner_Token_Type _check_keyword(int start, int length, const char* rest, Scanner_Token_Type type) {
    if(scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static Scanner_Token _token_error(const char* msg) {
    Scanner_Token token;
    token.type   = TOKEN_ERROR;
    token.start  = msg;
    token.length = (int) strlen(msg);
    token.line   = scanner.line;
    return token;
}
