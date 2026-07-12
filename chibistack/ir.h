// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef enum : u8 {
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
   IrInstrKind kind;
   u8 variant;
   u16 args;
} IrInstr;

typedef struct {
   Vector IrInstructions;
} IR;

const cstr IrInstrKind_to_cstr(IrInstrKind self);

IR IR_from_file(cstr file);
void IR_delete(IR* self);

void IR_dump(IR* self);

