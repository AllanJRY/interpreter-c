#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "compiler.h"
#include "memory.h"
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

typedef struct Local {
    Scanner_Token name;
    int           depth;
    bool          is_captured;
} Local;

typedef struct Upvalue {
    uint8_t idx;
    bool    is_local;
} Upvalue;

typedef enum Function_Type {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT,
} Function_Type;

typedef struct Compiler {
    struct Compiler* enclosing;
    Obj_Function*    function;
    Function_Type    type;
    Local            locals[UINT8_COUNT];
    int              local_count;
    Upvalue          upvalues[UINT8_COUNT];
    int              scope_depth;
} Compiler;

typedef struct Class_Compiler {
    struct Class_Compiler* enclosing;
} Class_Compiler;

static Parse_Rule* _parse_rule_get(Scanner_Token_Type token_type);

static void _error(const char* msg);
static void _error_at_current(const char* msg);
static void _error_at(Scanner_Token* token, const char* msg);
static void _synchronize_on_panic(void);

static uint8_t _make_constant(Value value);
static void    _parse_precedence(Precedence precedence);
static uint8_t _variable_parse(const char* error_msg);
static void    _variable_declare(void);
static void    _variable_define(uint8_t global_var_idx);
static void    _variable_named(Scanner_Token name, bool can_assign);
static void    _variable_mark_initialized(void);
static uint8_t _constant_identifier(Scanner_Token* name);
static bool    _match(Scanner_Token_Type type);
static bool    _check(Scanner_Token_Type type);
static bool    _identifiers_equal(Scanner_Token* a, Scanner_Token* b);
static uint8_t _argument_list(void);

static void _declaration(void);
static void _declaration_var(void);
static void _declaration_fun(void);
static void _declaration_class(void);
static void _statement(void);
static void _statement_print(void);
static void _statement_for(void);
static void _statement_if(void);
static void _statement_return(void);
static void _statement_while(void);
static void _statement_expression(void);
static void _expression(void);
static void _number(bool can_assign);
static void _grouping(bool can_assign);
static void _unary(bool can_assign);
static void _binary(bool can_assign);
static void _literal(bool can_assign);
static void _string(bool can_assign);
static void _variable(bool can_assign);
static void _and(bool can_assign);
static void _or(bool can_assign);
static void _block(void);
static void _scope_begin(void);
static void _scope_end(void);
static void _function(Function_Type type);
static void _function_call(bool can_assign);
static void _method(void);
static void _dot(bool can_assign);
static void _this(bool can_assign);


static void          _compiler_init(Compiler* compiler, Function_Type type);
static Obj_Function* _compiler_end(void);
static void          _compiler_emit_byte(uint8_t byte);
static void          _compiler_emit_bytes(uint8_t byte1, uint8_t byte2);
static void          _compiler_emit_return(void);
static void          _compiler_emit_constant(Value value);
static int           _compiler_emit_jump(uint8_t instruction);
static void          _compiler_emit_loop(int loop_start);
static Chunk*        _compiler_current_chunk(void);

static void _local_add(Scanner_Token name);
static int  _local_resolve(Compiler* compiler, Scanner_Token* name);

static int _upvalue_add(Compiler* compiler, uint8_t idx, bool is_local);
static int _upvalue_resolve(Compiler* compiler, Scanner_Token* name);

static void _jump_patch(int offset);

static void _parser_advance(void);
static void _parser_consume(Scanner_Token_Type type, const char* msg);

Parser parser;
Compiler* current_compiler    = NULL;
Class_Compiler* current_class = NULL;
Chunk* compiling_chunk;

Parse_Rule rules[] = {
    [TOKEN_LEFT_PAREN]    = {_grouping, _function_call, PREC_CALL},
    [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,           PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,      NULL,           PREC_NONE},
    [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,           PREC_NONE},
    [TOKEN_COMMA]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_DOT]           = {NULL,      _dot,           PREC_CALL},
    [TOKEN_MINUS]         = {_unary,    _binary,        PREC_TERM},
    [TOKEN_PLUS]          = {NULL,      _binary,        PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,      NULL,           PREC_NONE},
    [TOKEN_SLASH]         = {NULL,      _binary,        PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,      _binary,        PREC_FACTOR},
    [TOKEN_BANG]          = {_unary,    NULL,           PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,      _binary,        PREC_EQUALITY},
    [TOKEN_EQUAL]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,      _binary,        PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,      _binary,        PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,      _binary,        PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,      _binary,        PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,      _binary,        PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {_variable, NULL,           PREC_NONE},
    [TOKEN_STRING]        = {_string,   NULL,           PREC_NONE},
    [TOKEN_NUMBER]        = {_number,   NULL,           PREC_NONE},
    [TOKEN_AND]           = {NULL,      _and,           PREC_AND},
    [TOKEN_CLASS]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_ELSE]          = {NULL,      NULL,           PREC_NONE},
    [TOKEN_FALSE]         = {_literal,  NULL,           PREC_NONE},
    [TOKEN_FOR]           = {NULL,      NULL,           PREC_NONE},
    [TOKEN_FUN]           = {NULL,      NULL,           PREC_NONE},
    [TOKEN_IF]            = {NULL,      NULL,           PREC_NONE},
    [TOKEN_NIL]           = {_literal,  NULL,           PREC_NONE},
    [TOKEN_OR]            = {NULL,      _or,            PREC_OR},
    [TOKEN_PRINT]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_RETURN]        = {NULL,      NULL,           PREC_NONE},
    [TOKEN_SUPER]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_THIS]          = {_this,     NULL,           PREC_NONE},
    [TOKEN_TRUE]          = {_literal,  NULL,           PREC_NONE},
    [TOKEN_VAR]           = {NULL,      NULL,           PREC_NONE},
    [TOKEN_WHILE]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_ERROR]         = {NULL,      NULL,           PREC_NONE},
    [TOKEN_EOF]           = {NULL,      NULL,           PREC_NONE},
};

Obj_Function* compiler_compile(const char* source) {
    scanner_init(source);
    Compiler compiler;
    _compiler_init(&compiler, TYPE_SCRIPT);

    parser.had_error  = false;
    parser.panic_mode = false;

    _parser_advance();
    while(!_match(TOKEN_EOF)) {
        _declaration();
    }

    Obj_Function* function = _compiler_end();
    return parser.had_error ? NULL : function;
}

void mark_compiler_roots(void) {
    Compiler* compiler = current_compiler;
    while(compiler != NULL) {
        mark_object((Obj*) compiler->function);
        compiler = compiler->enclosing;
    }
}

static void _declaration(void) {
    if (_match(TOKEN_CLASS)) {
        _declaration_class();
    } else if (_match(TOKEN_FUN)) {
        _declaration_fun();
    } else if (_match(TOKEN_VAR)) {
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

static void _declaration_fun(void) {
    uint8_t global = _variable_parse("Expect function name.");
    _variable_mark_initialized();
    _function(TYPE_FUNCTION);
    _variable_define(global);
}

static void _declaration_class(void) {
    _parser_consume(TOKEN_IDENTIFIER, "Expect class name.");
    Scanner_Token class_name = parser.previous;
    uint8_t name_constant = _constant_identifier(&parser.previous);
    _variable_declare();

    _compiler_emit_bytes(OP_CLASS, name_constant);
    _variable_define(name_constant);

    Class_Compiler class_compiler;
    class_compiler.enclosing = current_class;
    current_class            = &class_compiler;

    _variable_named(class_name, false);
    _parser_consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while(!_check(TOKEN_RIGHT_BRACE) && !_check(TOKEN_EOF)) {
        _method();
    }
    _parser_consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

    _compiler_emit_byte(OP_POP);
    current_class = current_class->enclosing;
}

static uint8_t _variable_parse(const char* error_msg) {
    _parser_consume(TOKEN_IDENTIFIER, error_msg);
    _variable_declare();
    if (current_compiler->scope_depth > 0) return 0;
    return _constant_identifier(&parser.previous);
}

static uint8_t _constant_identifier(Scanner_Token* name) {
    return _make_constant(V_OBJ(string_copy(name->start, name->length)));
}

static void _variable_declare(void) {
    if (current_compiler->scope_depth == 0) return;
    Scanner_Token* name = &parser.previous;

    // Check for variable in the same scope.
    for (int i = current_compiler->local_count - 1; i >= 0; i -= 1) {
        Local* local = &current_compiler->locals[i];

        if (local->depth != -1 && local->depth < current_compiler->scope_depth) {
            break;
        }

        if (_identifiers_equal(name, &local->name)) {
            _error("Already a variable with this name in this scope");
        }
    }

    _local_add(*name);
}

static void _variable_define(uint8_t global_var_idx) {
    if (current_compiler->scope_depth > 0) {
        _variable_mark_initialized();
        return;
    }
    _compiler_emit_bytes(OP_DEFINE_GLOBAL, global_var_idx);
}

static bool _identifiers_equal(Scanner_Token* a, Scanner_Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, b->length) == 0;
}

static void _local_add(Scanner_Token name) {
    if (current_compiler->local_count == UINT8_COUNT) {
        _error("Too many local variable in function.");
        return;
    }
    Local* local       = &current_compiler->locals[current_compiler->local_count++];
    local->name        = name;
    local->depth       = -1;
    local->is_captured = false;
}

static int _local_resolve(Compiler* compiler, Scanner_Token* name) {
    for (int i = compiler->local_count - 1; i >= 0; i -= 1) {
        Local* local = &compiler->locals[i];
        if(_identifiers_equal(name, &local->name)) {
            if (local->depth == -1) {
                _error("Can't read local variable in its own initializer");
            }
            return i;
        }
    }

    return -1;
}

static void _variable_mark_initialized(void) {
    if (current_compiler->scope_depth == 0) return;
    current_compiler->locals[current_compiler->local_count - 1].depth = current_compiler->scope_depth;
}

static int _upvalue_add(Compiler* compiler, uint8_t idx, bool is_local) {
    int upvalue_count = compiler->function->upvalue_count;

    for (int i = 0; i < upvalue_count; i += 1) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->idx == idx && upvalue->is_local == is_local) {
            return i;
        }
    }

    if (upvalue_count == UINT8_COUNT) {
        _error("Too many closure variables in function");
        return 0;
    }

    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].idx      = idx;
    return compiler->function->upvalue_count++;
}

static int _upvalue_resolve(Compiler* compiler, Scanner_Token* name) {
    if (compiler->enclosing == NULL) {
        return -1;
    }

    int local = _local_resolve(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].is_captured = true;
        return _upvalue_add(compiler, (uint8_t) local, true);
    }

    int upvalue = _upvalue_resolve(compiler->enclosing, name);
    if(upvalue != -1) {
        return _upvalue_add(compiler, (uint8_t) upvalue, false);
    }

    return -1;
}

static void _statement(void) {
    if(_match(TOKEN_PRINT)) {
        _statement_print();
    } else if(_match(TOKEN_FOR)) {
        _statement_for();
    } else if(_match(TOKEN_IF)) {
        _statement_if();
    } else if(_match(TOKEN_RETURN)) {
        _statement_return();
    } else if(_match(TOKEN_WHILE)) {
        _statement_while();
    } else if(_match(TOKEN_LEFT_BRACE)) {
        _scope_begin();
        _block();
        _scope_end();
    } else {
        _statement_expression();
    }
}

static void _statement_print(void) {
    _expression();
    _parser_consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    _compiler_emit_byte(OP_PRINT);
}

static void _statement_for(void) {
    _scope_begin();

    _parser_consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    if (_match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (_match(TOKEN_VAR)) {
        _declaration_var();
    } else {
        _statement_expression();
    }

    int loop_start = _compiler_current_chunk()->len;
    int exit_jump = -1;
    if (!_match(TOKEN_SEMICOLON)) {
        _expression();
        _parser_consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        // Jump out of the loop if the condition is false.
        exit_jump = _compiler_emit_jump(OP_JUMP_IF_FALSE);
        _compiler_emit_byte(OP_POP); // Condition.
    }


    if (!_match(TOKEN_RIGHT_PAREN)) {
        int body_jump = _compiler_emit_jump(OP_JUMP);
        int increment_start = _compiler_current_chunk()->len;
        _expression();
        _compiler_emit_byte(OP_POP);

        _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        _compiler_emit_loop(loop_start);
        loop_start = increment_start;
        _jump_patch(body_jump);
    }

    _statement();

    _compiler_emit_loop(loop_start);
    if (exit_jump != -1) {
        _jump_patch(exit_jump);
        _compiler_emit_byte(OP_POP); // Condition.
    }

    _scope_end();
}

static void _statement_if(void) {
    _parser_consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    _expression();
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int then_jump = _compiler_emit_jump(OP_JUMP_IF_FALSE);
    _compiler_emit_byte(OP_POP);

    _statement();
    int else_jump = _compiler_emit_jump(OP_JUMP);

    _jump_patch(then_jump);
    _compiler_emit_byte(OP_POP);

    if (_match(TOKEN_ELSE)) _statement();
    _jump_patch(else_jump);
}

static void _statement_return(void) {
    if (current_compiler->type == TYPE_SCRIPT) {
        _error("Can't return from top-level code.");
    }

    if(_match(TOKEN_SEMICOLON)) {
        _compiler_emit_return();
    } else {
        if (current_compiler->type == TYPE_INITIALIZER) {
            _error("Can't return a value from an initializer.");
        }

        _expression();
        _parser_consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        _compiler_emit_byte(OP_RETURN);
    }
}

static void _statement_while(void) {
    int loop_start = _compiler_current_chunk()->len;
    _parser_consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    _expression();
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = _compiler_emit_jump(OP_JUMP_IF_FALSE);
    _compiler_emit_byte(OP_POP);

    _statement();
    _compiler_emit_loop(loop_start);

    _jump_patch(exit_jump);
    _compiler_emit_byte(OP_POP);
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
    (void) can_assign;
    double value = strtod(parser.previous.start, NULL);
    _compiler_emit_constant(V_NUMBER(value));
}

static void _grouping(bool can_assign) {
    (void) can_assign;
    _expression();
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void _unary(bool can_assign) {
    (void) can_assign;
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
    (void) can_assign;
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
    (void) can_assign;
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
    (void) can_assign;
    _compiler_emit_constant(V_OBJ(string_copy(parser.previous.start + 1, parser.previous.length - 2)));
}

static void _variable(bool can_assign) {
    _variable_named(parser.previous, can_assign);
}

static void _variable_named(Scanner_Token name, bool can_assign) {
    uint8_t get_op, set_op;

    int arg = _local_resolve(current_compiler, &name);
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else if ((arg = _upvalue_resolve(current_compiler, &name)) != -1) {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    } else {
        arg = _constant_identifier(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && _match(TOKEN_EQUAL)) {
        _expression();
        _compiler_emit_bytes(set_op, (uint8_t)arg);
    } else {
        _compiler_emit_bytes(get_op, (uint8_t)arg);
    }
}

static void _and(bool can_assign) {
    (void) can_assign;
    int end_jump = _compiler_emit_jump(OP_JUMP_IF_FALSE);

    _compiler_emit_byte(OP_POP);
    _parse_precedence(PREC_AND);

    _jump_patch(end_jump);
}

static void _or(bool can_assign) {
    (void) can_assign;
    int else_jump = _compiler_emit_jump(OP_JUMP_IF_FALSE);
    int end_jump  = _compiler_emit_jump(OP_JUMP);

    _jump_patch(else_jump);
    _compiler_emit_byte(OP_POP);

    _parse_precedence(PREC_OR);
    _jump_patch(end_jump);
}

static void _block(void) {
    while(!_check(TOKEN_RIGHT_BRACE) && !_check(TOKEN_EOF)) {
        _declaration();
    }

    _parser_consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void _scope_begin(void) {
    current_compiler->scope_depth += 1;
}

static void _scope_end(void) {
    current_compiler->scope_depth -= 1;
    while(current_compiler->local_count > 0 && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth) {
        if (current_compiler->locals[current_compiler->local_count - 1].is_captured) {
            _compiler_emit_byte(OP_CLOSE_UPVALUE);
        } else {
            _compiler_emit_byte(OP_POP);
        }
        current_compiler->local_count -= 1;
    }
}

static void _function(Function_Type type) {
    Compiler compiler;
    _compiler_init(&compiler, type);
    _scope_begin();

    _parser_consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!_check(TOKEN_RIGHT_PAREN)) {
        do {
            current_compiler->function->arity += 1;
            if (current_compiler->function->arity > 255) {
                _error_at_current("Can't have more than 255 parameters");
            }
            uint8_t constant_idx = _variable_parse("Expect parameter name");
            _variable_define(constant_idx);
        } while(_match(TOKEN_COMMA));
    }
    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    _parser_consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");

    _block();
    Obj_Function* function = _compiler_end(); // No _scope_end call needed because of this call.

    _compiler_emit_bytes(OP_CLOSURE, _make_constant(V_OBJ(function)));
    for (int i = 0; i < function->upvalue_count; i += 1) {
        _compiler_emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        _compiler_emit_byte(compiler.upvalues[i].idx);
    }
}

static void _function_call(bool can_assign) {
    (void) can_assign;
    uint8_t arg_count = _argument_list();
    _compiler_emit_bytes(OP_CALL, arg_count);
}

static void _method(void) {
    _parser_consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant_idx = _constant_identifier(&parser.previous);
    Function_Type type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    _function(type);
    _compiler_emit_bytes(OP_METHOD, constant_idx);
}

static uint8_t _argument_list(void) {
    uint8_t arg_count = 0;
    if (!_check(TOKEN_RIGHT_PAREN)) {
        do {
            _expression();
            if (arg_count == 255) {
                _error("Can't have more than 255 arguments.");
            }
            arg_count += 1;
        } while(_match(TOKEN_COMMA));
    }

    _parser_consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void _dot(bool can_assign) {
    _parser_consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name_constant = _constant_identifier(&parser.previous);
    if (can_assign && _match(TOKEN_EQUAL)) {
        _expression();
        _compiler_emit_bytes(OP_SET_PROPERTY, name_constant);
    } else if(_match(TOKEN_LEFT_PAREN)) {
        uint8_t arg_count = _argument_list();
        _compiler_emit_bytes(OP_INVOKE, name_constant);
        _compiler_emit_byte(arg_count);
    } else {
        _compiler_emit_bytes(OP_GET_PROPERTY, name_constant);
    }
}

static void _this(bool can_assign) {
    (void) can_assign;

    if(current_class == NULL) {
        _error("Can't use 'this' outside of a class.");
        return;
    }

    _variable(false);
}

static void _compiler_init(Compiler* compiler, Function_Type type) {
    compiler->enclosing   = current_compiler;
    compiler->function    = NULL;
    compiler->type        = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function    = function_new();
    current_compiler      = compiler;
    if (type != TYPE_SCRIPT) {
        current_compiler->function->name = string_copy(parser.previous.start, parser.previous.length);
    }
    Local* local          = &current_compiler->locals[current_compiler->local_count++];
    local->depth          = 0;
    local->is_captured    = false;
    if (type != TYPE_FUNCTION) {
        local->name.start     = "this";
        local->name.length    = 4;
    } else {
        local->name.start     = "";
        local->name.length    = 0;
    }
}

static Obj_Function* _compiler_end(void) {
    _compiler_emit_return();
    Obj_Function* function = current_compiler->function;

    #ifdef DEBUG_PRINT_CODE
    if (!parser.had_error) {
        chunk_disassemble(_compiler_current_chunk(), function->name != NULL ? function->name->chars : "<script>");
    }
    #endif

    current_compiler = current_compiler->enclosing;
    return function;
}

static void _compiler_emit_byte(uint8_t byte) {
    chunk_write(_compiler_current_chunk(), byte, parser.previous.line);
}

static void _compiler_emit_bytes(uint8_t byte1, uint8_t byte2) {
    _compiler_emit_byte(byte1);
    _compiler_emit_byte(byte2);
}

static void _compiler_emit_return(void) {
    if (current_compiler->type == TYPE_INITIALIZER) {
        _compiler_emit_bytes(OP_GET_LOCAL, 0);
    } else {
        _compiler_emit_byte(OP_NIL);
    }
    
    _compiler_emit_byte(OP_RETURN);
}

static void _compiler_emit_constant(Value value) {
    _compiler_emit_bytes(OP_CONSTANT, _make_constant(value));
}

static int _compiler_emit_jump(uint8_t instruction) {
    _compiler_emit_byte(instruction);
    _compiler_emit_byte(0xff);
    _compiler_emit_byte(0xff);
    return _compiler_current_chunk()->len - 2;
}

static void _compiler_emit_loop(int loop_start) {
    _compiler_emit_byte(OP_LOOP);

    int offset = _compiler_current_chunk()->len - loop_start + 2;
    if (offset > UINT16_MAX) _error("Loop body too large.");

    _compiler_emit_byte((offset >> 8) & 0xff);
    _compiler_emit_byte(offset & 0xff);
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
    return &current_compiler->function->chunk;
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

static void _jump_patch(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = _compiler_current_chunk()->len - offset - 2;

    if (jump > UINT16_MAX) {
        _error("too much code to jump over");
    }

    _compiler_current_chunk()->code[offset]     = (jump >> 8) & 0xff;
    _compiler_current_chunk()->code[offset + 1] = jump & 0xff;

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
