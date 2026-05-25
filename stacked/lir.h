#pragma once

typedef enum {
   BIT32
} BitLength;

typedef enum {
   IT_Push,
   IT_Add,
   IT_Sub,
   IT_Mul,
   IT_Div,
   IT_Print
} InstrType;

typedef struct {
   struct {
      usize x;
      usize y;
      usize length;
      cstr path;
   } origin;

   InstrType type;
   union {
      struct {
         bool is_signed;
         BitLength bits;
      } bit_variant;
   }
} Instruction;

/// A [Vector] of [Instruction]s
typedef Vector LIR;

LIR LIR_from_lexer(Lexer* lexer);
void LIR_delete(nullable LIR* self);

void LIR_display(LIR* self);

