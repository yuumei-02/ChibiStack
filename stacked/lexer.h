// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef struct : i32 {
   TT_Eof,
   
   TT_Add,
   TT_Sub,
   TT_Div,
   TT_Mul,

   TT_Word
} TokenType;

typedef struct {
   TokenType type;
   u32 z;

   union {
      i64 int_literal;
      StringView str_literal;
   };
} Token;

typedef enum {
   LM_Trim,
   LM_Word,
   LM_IntLiteral,
} LexerMode;

typedef struct {
   u32 z;

   cstr file_path;
   cstr file_contents;
} Lexer;

Lexer Lexer_new(cstr file_path);
void Lexer_delete(Lexer* self);

Token Lexer_next(Lexer* self);

