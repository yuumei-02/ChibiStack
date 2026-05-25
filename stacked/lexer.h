#pragma once

typedef enum {
   // Misc
   TT_Eof,

   // Operators
   TT_Add,
   TT_Sub,
   TT_Mul,
   TT_Div,

   // Literals
   TT_Identifier,
   TT_StrLiteral,
   TT_IntLiteral,

   // Keywords
   TT_Drop,
   TT_Swap,
   TT_Dup,
   TT_Puti,
   TT_Puts,
} TokenType;

// @note: don't forget to update Token_free when adding new tokens
typedef struct {
   usize x;
   usize y;
   usize length;
   TokenType type;

   union {
      i64 int_literal;
      String str_literal;
   };
} Token;

typedef enum {
   LM_Trim,
   LM_Normal,
   LM_Integer,
   LM_String,
   LM_Comment,
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
void Token_display(const cstr path, Token self);
void Token_free(Token token);

Lexer Lexer_new(cstr filepath);
void Lexer_delete(nullable Lexer* self);

Token Lexer_next(Lexer* self);

