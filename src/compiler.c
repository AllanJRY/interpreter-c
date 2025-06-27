#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct Parser {
    Scanner_Token current;
    Scanner_Token previous;
    bool          had_error;
    bool          panic_mode;
} Parser;

typedef enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // <> <= >=
    PREC_TERM,       // + =
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY,
} Precedence;

typedef void (*Parse_Fn)(void);

typedef struct Parse_Rule {
    Parse_Fn   prefix;
    Parse_Fn   infix;
    Precedence precedence;
} Parse_Rule;

static Parse_Rule* _parse_rule_get(Scanner_Token_Type token_type);

static void _error(const char* msg);
static void _error_at_current(const char* msg);
static void _error_at(Scanner_Token* token, const char* msg);

static uint8_t _make_constant(Value value);
static void    _parse_precedence(Precedence precedence);

static void _number(void);
static void _grouping(void);
static void _unary(void);
static void _binary(void);
static void _expression(void);

static void   _compiler_end(void);
static void   _compiler_emit_byte(uint8_t byte);
static void   _compiler_emit_bytes(uint8_t byte1, uint8_t byte2);
static void   _compiler_emit_return(void);
static void   _compiler_emit_constant(Value value);
static Chunk* _compiler_current_chunk(void);

static void _parser_advance(void);
static void _parser_consume(Scanner_Token_Type type, const char* msg);

Parser parser;
Chunk* compiling_chunk;

Parse_Rule rules[] = {
    [TOKEN_LEFT_PAREN]    = {_grouping, NULL,    PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,    PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,      NULL,    PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,    PREC_NONE},
    [TOKEN_COMMA]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_DOT]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_MINUS]         = {_unary,    _binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,      _binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,      NULL,    PREC_NONE},
    [TOKEN_SLASH]         = {NULL,      _binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,      _binary, PREC_FACTOR},
    [TOKEN_BANG]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,      NULL,    PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,      NULL,    PREC_NONE},
    [TOKEN_GREATER]       = {NULL,      NULL,    PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,      NULL,    PREC_NONE},
    [TOKEN_LESS]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,      NULL,    PREC_NONE},
    [TOKEN_IDENTIFIER]    = {NULL,      NULL,    PREC_NONE},
    [TOKEN_STRING]        = {NULL,      NULL,    PREC_NONE},
    [TOKEN_NUMBER]        = {_number,   NULL,    PREC_NONE},
    [TOKEN_AND]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_CLASS]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_ELSE]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_FALSE]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_FOR]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_FUN]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_IF]            = {NULL,      NULL,    PREC_NONE},
    [TOKEN_NIL]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_OR]            = {NULL,      NULL,    PREC_NONE},
    [TOKEN_PRINT]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_RETURN]        = {NULL,      NULL,    PREC_NONE},
    [TOKEN_SUPER]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_THIS]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_TRUE]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_VAR]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_WHILE]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_ERROR]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_EOF]           = {NULL,      NULL,    PREC_NONE},
};

bool compiler_compile(const char* source, Chunk* chunk) {
    scanner_init(source);
    compiling_chunk   = chunk;
    parser.had_error  = false;
    parser.panic_mode = false;

    _parser_advance();
    _expression();
    _parser_consume(TOKEN_EOF, "Expect end of expression.");
    _compiler_end();
    return !parser.had_error;
}

static Parse_Rule* _parse_rule_get(Scanner_Token_Type token_type) {
    return &rules[token_type];
}

static void _number(void) {
    double value = strtod(parser.previous.start, NULL);
    _compiler_emit_constant(value);
}

static void _grouping(void) {
    _expression();
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void _unary(void) {
    Scanner_Token_Type operator_type = parser.previous.type;

    // Compile the operand.
    _parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch(operator_type) {
        case TOKEN_MINUS: {
            _compiler_emit_byte(OP_NEGATE);
            break;
        } 
        default: return; // Unreachable

    }
}

static void _binary(void) {
    Scanner_Token_Type operator_type = parser.previous.type;
    Parse_Rule* rule = _parse_rule_get(operator_type);
    _parse_precedence((Precedence) (rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_PLUS: {
            _compiler_emit_byte(OP_ADD);
            break;
        }
        case TOKEN_MINUS: {
            _compiler_emit_byte(OP_SUBTRACT);
            break;
        }
        case TOKEN_STAR: {
            _compiler_emit_byte(OP_MULTIPLY);
            break;
        }
        case TOKEN_SLASH: {
            _compiler_emit_byte(OP_DIVIDE);
            break;
        }
        default : return; // Unreachable.
    }
}

static void _parse_precedence(Precedence precedence) {
    _parser_advance();
    Parse_Fn prefix_rule = _parse_rule_get(parser.previous.type)->prefix;

    if(prefix_rule == NULL) {
        _error("Expect expression.");
        return;
    }

    prefix_rule();

    while(precedence <= _parse_rule_get(parser.current.type)->precedence) {
        _parser_advance();
        Parse_Fn infix_rule = _parse_rule_get(parser.previous.type)->infix;
        infix_rule();
    }
}

static void _expression(void) {
    _parse_precedence(PREC_ASSIGNMENT);
}

static void _compiler_end(void) {
    _compiler_emit_return();
    #ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        chunk_disassemble(_compiler_current_chunk(), "code");
    }
    #endif
}

static void _compiler_emit_byte(uint8_t byte) {
    chunk_write(_compiler_current_chunk(), byte, parser.previous.line);
}

static void _compiler_emit_bytes(uint8_t byte1, uint8_t byte2) {
    _compiler_emit_byte(byte1);
    _compiler_emit_byte(byte2);
}

static void _compiler_emit_return(void) {
    _compiler_emit_byte(OP_RETURN);
}

static void _compiler_emit_constant(Value value) {
    _compiler_emit_bytes(OP_CONSTANT, _make_constant(value));
}

static uint8_t _make_constant(Value value) {
    int constant_idx = chunk_constants_add(_compiler_current_chunk(), value);

    if (constant_idx > UINT8_MAX) {
        _error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t) constant_idx;
}

static Chunk* _compiler_current_chunk(void) {
    return compiling_chunk;
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

