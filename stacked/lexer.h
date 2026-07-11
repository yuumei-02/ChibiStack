// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef enum : i32 {
   TT_Eof,
   
   TT_Add,
   TT_Sub,
   TT_Idiv,
   TT_Imul,
   TT_Udiv,
   TT_Umul,

   TT_Word,
   TT_IntLiteral,

   // Temporary instrinsic procedure
   TT_Puti
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
   LM_Comment
} LexerMode;

typedef struct {
   cstr file_path;
   cstr file_contents;

   u32 z;
   i32 current;
   i32 peek;
} Lexer;

Lexer Lexer_new(cstr file_path);
void Lexer_delete(Lexer* self);

Token Lexer_next(Lexer* self);

void Token_dump(Token self, Lexer* lexer);

const cstr TokenType_to_cstr(TokenType self);

