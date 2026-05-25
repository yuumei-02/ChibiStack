#pragma once

typedef enum {
   // Misc
   TT_Eof,

   // Operators
   TT_Plus,

   // Literals
   TT_IntLiteral,

   // Keywords
   TT_Print,
} TokenType;

typedef struct {
   usize x;
   usize y;
   usize length;
   TokenType type;

   union {
      i64 int_literal;
   };
} Token;

typedef enum {
   LM_Trim,
   LM_Normal,
   LM_Integer,
} LexerMode;

typedef struct {
   usize x;
   usize y;
   LexerMode mode;

   i32 current;
   i32 peek;
   String accumulated;

   struct {
      cstr path;
      FILE* handle;
   } file;
} Lexer;

const cstr TokenType_to_cstr(TokenType self);
void Token_display(Token self);

Lexer Lexer_new(cstr filepath);
void Lexer_delete(nullable Lexer* self);

Token Lexer_next(Lexer* self);

