// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "ir.h"

const cstr IrInstrKind_to_cstr(IrInstrKind self) {
   switch (self) {
      case IIK_PushInt:  return "PushInt";
      case IIK_PushUint: return "PushUint";
      case IIK_PushAddr: return "PushAddr";
      case IIK_Drop:     return "Drop";
      case IIK_Swap:     return "Swap";
      case IIK_Dup:      return "Dup";
      case IIK_Add:      return "Add";
      case IIK_Sub:      return "Sub";
      case IIK_Idiv:     return "Idiv";
      case IIK_Udiv:     return "Udiv";
      case IIK_Mul:      return "Mul";
      case IIK_Syscall4: return "Syscall4";
      case IIK_Puti:     return "Puti";
   }

   return "Unknown";
}

IR IR_from_file(cstr file) {
   mcu_assert(file != nullptr, "file can't be null");

   IR self = {
      .Lexers = Vector_new(sizeof(Lexer)),
      .IrInstructions = Vector_new(sizeof(IrInstr)),
      .string_literals = Vector_new(sizeof(String))
   };

   Vector_push_create(&self.Lexers, (Lexer_new(file)));
   u32 lexer_i = (u32) (self.Lexers.length - 1);
   Lexer* lexer = Vector_get(&self.Lexers, self.Lexers.length - 1);

   loop {
      Token token = Lexer_next(lexer);
      IrInstr instr = {
         .z = token.z,
         .lexer = lexer_i
      };

      #define push_instr(instruction_kind) { \
         instr.kind = instruction_kind; \
         Vector_push(&self.IrInstructions, &instr); \
      }

      switch (token.type) {
         case TT_IntLiteral: {
            instr.kind = IIK_PushInt;
            instr.int_value = token.int_literal;
            Vector_push(&self.IrInstructions, &instr);
         } continue;

         case TT_StrLiteral: {
            instr.kind = IIK_PushUint;
            instr.uint_value = token.str_literal.length;
            Vector_push(&self.IrInstructions, &instr);

            instr.kind = IIK_PushAddr;
            instr.uint_value = self.string_literals.length;
            Vector_push(&self.IrInstructions, &instr);

            Vector_push(&self.string_literals, &token.str_literal);
         } continue;

         case TT_Drop: push_instr(IIK_Drop) continue;
         case TT_Swap: push_instr(IIK_Swap) continue;
         case TT_Dup:  push_instr(IIK_Dup)  continue;

         case TT_Add:  push_instr(IIK_Add)  continue;
         case TT_Sub:  push_instr(IIK_Sub)  continue;
         case TT_Idiv: push_instr(IIK_Idiv) continue;
         case TT_Udiv: push_instr(IIK_Udiv) continue;
         case TT_Mul:  push_instr(IIK_Mul)  continue;

         case TT_Syscall4: push_instr(IIK_Syscall4) continue;

         case TT_Puti: push_instr(IIK_Puti) continue;

         case TT_Word: mcu_todo("not yet implemented");
         case TT_Eof:  goto finish_parsing;
      }

      panic("unreachable");
      #undef push_instr
   }

finish_parsing:
   return self;
}

void IR_delete(IR* self) {
   mcu_assert(self != nullptr, "self can't be null");

   foreach (self->Lexers, i) {
      Lexer* lexer = Vector_get(&self->Lexers, i);
      Lexer_delete(lexer);
   }

   foreach (self->string_literals, i) {
      String* str = Vector_get(&self->string_literals, i);
      String_free(str);
   }

   Vector_free(&self->Lexers);
   Vector_free(&self->string_literals);
   Vector_free(&self->IrInstructions);
   *self = (IR) {0};
}

void IR_dump(IR* self) {
   mcu_assert(self != nullptr, "self can't be null");

   // @todo: add location information
   foreach (self->IrInstructions, i) {
      IrInstr* instr = Vector_get(&self->IrInstructions, i);

      Lexer* lexer = Vector_get(&self->Lexers, instr->lexer);
      Loc loc = Lexer_loc_from_offset(lexer, instr->z);
      printf("%s:%u:%u: info: %s",
         lexer->file_path, loc.y, loc.x, IrInstrKind_to_cstr(instr->kind));

      switch (instr->kind) {
         case IIK_PushInt:  println(" (%ld)", instr->int_value);  break;
         case IIK_PushUint: println(" (%lu)", instr->uint_value); break;
         case IIK_PushAddr: println(" (%lu)", instr->uint_value); break;
         default: printf("\n");
      }
   }
}

