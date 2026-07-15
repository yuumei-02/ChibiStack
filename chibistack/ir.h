// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#pragma once

typedef enum : u16 {
   IIK_PushInt,
   IIK_PushUint,
   IIK_PushAddr,

   IIK_Drop,
   IIK_Swap,
   IIK_Dup,

   IIK_Add,
   IIK_Sub,
   IIK_Idiv,
   IIK_Udiv,
   IIK_Mul,

   IIK_Syscall0,
   IIK_Syscall1,
   IIK_Syscall2,
   IIK_Syscall3,
   IIK_Syscall4,
   IIK_Syscall5,
   IIK_Syscall6,

   IIK_ProcBegin,
   IIK_ProcEnd,
   IIK_ProcCall,

   IIK_Puti,

   IIK_InvalidSymbol
} IrInstrKind;

typedef struct {
   union {
      i64 int_value;
      u64 uint_value;
      StringView word;
   };

   u32 z;
   u16 lexer;
   IrInstrKind kind;
} IrInstr;

typedef enum {
   BIT64
} BitLength;

typedef enum {
   TK_Void,
   TK_Int,
   TK_Ptr,
   TK_Proc
} TypeKind;

typedef struct {
   TypeKind kind;
   u32 type_id;

   union {
      struct {
         bool is_signed;
         BitLength bits;
      } integer;

      struct {
         Vector parameter_types;
         Vector return_types;
      } proc;
   };
} Type;

typedef struct {
   u16 lexer_i;
   u32 id;
   u32 offset;
   cstr name;
} TypeInfo;

typedef enum {
   SK_Proc
} SymbolKind;

typedef struct {
   SymbolKind kind;

   union {
      struct {
         u32 type_id;
      } proc;
   };
} Symbol;

HashMap_hdr(Type)
HashMap_hdr(Symbol)

typedef struct {
   HashMap(Type) type_table;
   HashMap(Symbol) symbol_table;
   Vector Lexers;
   Vector IrInstructions;
   Vector string_literals;
} IR;

const cstr IrInstrKind_to_cstr(IrInstrKind self);

IR IR_from_file(cstr file, bool* failure, double* ir_time, double* lexer_time);
void IR_delete(IR* self);

void IR_dump(IR* self);

