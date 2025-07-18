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

typedef void (*Parse_Fn)(bool can_assign);

typedef struct Parse_Rule {
    Parse_Fn   prefix;
    Parse_Fn   infix;
    Precedence precedence;
} Parse_Rule;

static Parse_Rule* _parse_rule_get(Scanner_Token_Type token_type);

static void _error(const char* msg);
static void _error_at_current(const char* msg);
static void _error_at(Scanner_Token* token, const char* msg);
static void _synchronize_on_panic(void);

static uint8_t _make_constant(Value value);
static void    _parse_precedence(Precedence precedence);
static uint8_t _variable_parse(const char* error_msg);
static void    _variable_define(uint8_t global_var_idx);
static void    _variable_named(Scanner_Token name, bool can_assign);
static uint8_t _constant_identifier(Scanner_Token* name);
static bool    _match(Scanner_Token_Type type);
static bool    _check(Scanner_Token_Type type);

static void _declaration(void);
static void _declaration_var(void);
static void _statement(void);
static void _statement_print(void);
static void _statement_expression(void);
static void _expression(void);
static void _number(bool can_assign);
static void _grouping(bool can_assign);
static void _unary(bool can_assign);
static void _binary(bool can_assign);
static void _literal(bool can_assign);
static void _string(bool can_assign);
static void _variable(bool can_assign);

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
    [TOKEN_BANG]          = {_unary,    NULL,    PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,      _binary, PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,      _binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,      _binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,      _binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,      _binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,      _binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {_variable, NULL,    PREC_NONE},
    [TOKEN_STRING]        = {_string,   NULL,    PREC_NONE},
    [TOKEN_NUMBER]        = {_number,   NULL,    PREC_NONE},
    [TOKEN_AND]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_CLASS]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_ELSE]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_FALSE]         = {_literal,  NULL,    PREC_NONE},
    [TOKEN_FOR]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_FUN]           = {NULL,      NULL,    PREC_NONE},
    [TOKEN_IF]            = {NULL,      NULL,    PREC_NONE},
    [TOKEN_NIL]           = {_literal,  NULL,    PREC_NONE},
    [TOKEN_OR]            = {NULL,      NULL,    PREC_NONE},
    [TOKEN_PRINT]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_RETURN]        = {NULL,      NULL,    PREC_NONE},
    [TOKEN_SUPER]         = {NULL,      NULL,    PREC_NONE},
    [TOKEN_THIS]          = {NULL,      NULL,    PREC_NONE},
    [TOKEN_TRUE]          = {_literal,  NULL,    PREC_NONE},
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
    while(!_match(TOKEN_EOF)) {
        _declaration();
    }

    _compiler_end();

    return !parser.had_error;
}

static void _declaration(void) {
    if (_match(TOKEN_VAR)) {
        _declaration_var();
    } else {
        _statement();
    }
    if(parser.panic_mode) _synchronize_on_panic();
}

static void _declaration_var(void) {
    uint8_t global_var_idx = _variable_parse("expect variable name.");

    if (_match(TOKEN_EQUAL)) {
        _expression();
    } else {
        _compiler_emit_byte(OP_NIL);
    }

    _parser_consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    _variable_define(global_var_idx);
}

static uint8_t _variable_parse(const char* error_msg) {
    _parser_consume(TOKEN_IDENTIFIER, error_msg);
    return _constant_identifier(&parser.previous);
}

static uint8_t _constant_identifier(Scanner_Token* name) {
    return _make_constant(V_OBJ(string_copy(name->start, name->length)));
}

static void _variable_define(uint8_t global_var_idx) {
    _compiler_emit_bytes(OP_DEFINE_GLOBAL, global_var_idx);

}

static void _statement(void) {
    if(_match(TOKEN_PRINT)) {
        _statement_print();
    } else {
        _statement_expression();
    }
}

static void _statement_print(void) {
    _expression();
    _parser_consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    _compiler_emit_byte(OP_PRINT);
}

static void _statement_expression(void) {
    _expression();
    _parser_consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    _compiler_emit_byte(OP_POP);
}

static void _expression(void) {
    _parse_precedence(PREC_ASSIGNMENT);
}

static void _parse_precedence(Precedence precedence) {
    _parser_advance();
    Parse_Fn prefix_rule = _parse_rule_get(parser.previous.type)->prefix;

    if(prefix_rule == NULL) {
        _error("Expect expression.");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(can_assign);

    while(precedence <= _parse_rule_get(parser.current.type)->precedence) {
        _parser_advance();
        Parse_Fn infix_rule = _parse_rule_get(parser.previous.type)->infix;
        infix_rule(can_assign);
    }

    if (can_assign && _match(TOKEN_EQUAL)) {
        _error("Invalid assignment target.");
    }
}

static bool _match(Scanner_Token_Type type) {
    if(!_check(type)) return false;
    
    _parser_advance();

    return true;
}

static bool _check(Scanner_Token_Type type) {
    return parser.current.type == type;
}

static Parse_Rule* _parse_rule_get(Scanner_Token_Type token_type) {
    return &rules[token_type];
}

static void _number(bool can_assign) {
    double value = strtod(parser.previous.start, NULL);
    _compiler_emit_constant(V_NUMBER(value));
}

static void _grouping(bool can_assign) {
    _expression();
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void _unary(bool can_assign) {
    Scanner_Token_Type operator_type = parser.previous.type;

    // Compile the operand.
    _parse_precedence(PREC_UNARY);

    // Emit the operator instruction.
    switch(operator_type) {
        case TOKEN_BANG: {
            _compiler_emit_byte(OP_NOT);
            break;
        } 
        case TOKEN_MINUS: {
            _compiler_emit_byte(OP_NEGATE);
            break;
        } 
        default: return; // Unreachable

    }
}

static void _binary(bool can_assign) {
    Scanner_Token_Type operator_type = parser.previous.type;
    Parse_Rule* rule = _parse_rule_get(operator_type);
    _parse_precedence((Precedence) (rule->precedence + 1));

    switch (operator_type) {
        case TOKEN_BANG_EQUAL: {
            _compiler_emit_bytes(OP_EQUAL, OP_NOT);
            break;
        }
        case TOKEN_EQUAL_EQUAL: {
            _compiler_emit_byte(OP_EQUAL);
            break;
        }
        case TOKEN_GREATER: {
            _compiler_emit_byte(OP_GREATER);
            break;
        }
        case TOKEN_GREATER_EQUAL: {
            _compiler_emit_bytes(OP_LESS, OP_NOT);
            break;
        }
        case TOKEN_LESS: {
            _compiler_emit_byte(OP_LESS);
            break;
        }
        case TOKEN_LESS_EQUAL: {
            _compiler_emit_bytes(OP_GREATER, OP_NOT);
            break;
        }
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

static void _literal(bool can_assign) {
    switch(parser.previous.type) {
        case TOKEN_FALSE: {
            _compiler_emit_byte(OP_FALSE);
            break;
        }
        case TOKEN_NIL: {
            _compiler_emit_byte(OP_NIL);
            break;
        }
        case TOKEN_TRUE: {
            _compiler_emit_byte(OP_TRUE);
            break;
        }
        default: return; // Unreachable.
    }
}

static void _string(bool can_assign) {
    _compiler_emit_constant(V_OBJ(string_copy(parser.previous.start + 1, parser.previous.length - 2)));
}

static void _variable(bool can_assign) {
    _variable_named(parser.previous, can_assign);
}

static void _variable_named(Scanner_Token name, bool can_assign) {
    uint8_t arg = _constant_identifier(&name);
    if (can_assign && _match(TOKEN_EQUAL)) {
        _expression();
        _compiler_emit_bytes(OP_SET_GLOBAL, arg);
    } else {
        _compiler_emit_bytes(OP_GET_GLOBAL, arg);
    }
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

static void _synchronize_on_panic(void) {
    parser.panic_mode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default: ; // Do nothing.
        }

        _parser_advance();
    }
}
