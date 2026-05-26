#pragma once

typedef enum {
   IT_PushStr,
   IT_Push,
   IT_Pop,
   IT_Swap,
   IT_Dup,

   IT_Label,
   IT_CJmp,
   
   IT_Add, IT_Sub,
   IT_Mul, IT_Div,

   IT_Equ,     IT_NotEqu,
   IT_Less,    IT_More,
   IT_LessEqu, IT_MoreEqu,
   IT_Not,

   IT_Syscall,
   IT_Puti,
   IT_Puts
} InstrType;

typedef enum : i64 {
   LT_If,
   LT_End
} LabelType;

typedef struct {
   struct {
      usize x;
      usize y;
      usize length;
      cstr path;
   } origin;

   InstrType type;
   i64 args[1];
} Instruction;

/// A [Vector] of [Instruction]s
typedef struct {
   Vector Instructions;
   Vector StrLiterals;
} LIR;

LIR LIR_from_lexer(Lexer* lexer);
void LIR_delete(nullable LIR* self);

void LIR_path_labels(nullable LIR* self);
void LIR_display(LIR* self);

