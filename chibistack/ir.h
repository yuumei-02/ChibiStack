// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef enum : u16 {
   IIK_Push,

   IIK_Add,
   IIK_Sub,
   IIK_Idiv,
   IIK_Imul,
   IIK_Udiv,
   IIK_Umul,

   IIK_Puti
} IrInstrKind;

typedef struct {
   union {
      i64 int_value;
      u64 uint_value;
   };

   u32 z;
   u16 lexer;
   IrInstrKind kind;
} IrInstr;

typedef struct {
   Vector Lexers;
   Vector IrInstructions;
} IR;

const cstr IrInstrKind_to_cstr(IrInstrKind self);

IR IR_from_file(cstr file);
void IR_delete(IR* self);

void IR_dump(IR* self);

