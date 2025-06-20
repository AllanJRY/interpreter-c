#ifndef INTERP_SCANNER_H

typedef enum Scanner_Token_Type {
    // Single character tokens.
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN, 
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE, 
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,

    // One OR two character tokens.
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    TOKEN_ERROR,
    TOKEN_EOF,
} Scanner_Token_Type;

typedef struct Scanner_Token {
    Scanner_Token_Type type;
    const char*        start;
    int                length;
    int                line;
} Scanner_Token;

void scanner_init(const char* source);
Scanner_Token scanner_scan_token(void);

#define INTERP_SCANNER_H
#endif
