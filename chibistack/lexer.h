// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef struct {
   u32 x;
   u32 y;
} Loc;

typedef enum : i32 {
   TT_Eof,

   // Stack manipulators
   TT_Drop,
   TT_Swap,
   TT_Dup,

   // Operators
   TT_Add,
   TT_Sub,
   TT_Idiv,
   TT_Udiv,
   TT_Mul,

   // Literals
   TT_Word,
   TT_IntLiteral,
   TT_StrLiteral,

   // Intrinsiccs
   TT_Syscall0,
   TT_Syscall1,
   TT_Syscall2,
   TT_Syscall3,
   TT_Syscall4,
   TT_Syscall5,
   TT_Syscall6,

   // Keywords
   TT_Proc,
   TT_Begin,
   TT_End,

   // Compiler directives
   TT_Import,

   // Temporary instrinsic procedures
   TT_Puti
} TokenType;

typedef struct {
   TokenType type;
   u32 z;

   union {
      i64 int_literal;
      StringView str_view;
      String str_literal;
   };
} Token;

typedef enum {
   LM_Trim,
   LM_Word,
   LM_IntLiteral,
   LM_StrLiteral,
   LM_Comment
} LexerMode;

typedef struct {
   Vector new_line_indices;
   cstr file_path;
   cstr full_path;
   cstr file_contents;
   u32 z;
   i32 current;
   i32 peek;
} Lexer;

Lexer Lexer_new(cstr file_path);
void Lexer_delete(Lexer* self);

Token Lexer_next(Lexer* self, double* lexer_time);
Loc Lexer_loc_from_offset(Lexer* self, u32 offset);

u32 get_tokens_parsed();

void Token_dump(Token self, Lexer* lexer);

const cstr TokenType_to_cstr(TokenType self);

