// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "ir.h"

const cstr IrInstrKind_to_cstr(IrInstrKind self) {
   switch (self) {
      case IIK_Push: return "Push";
      case IIK_Add:  return "Add";
      case IIK_Sub:  return "Sub";
      case IIK_Idiv: return "Idiv";
      case IIK_Imul: return "Imul";
      case IIK_Udiv: return "Udiv";
      case IIK_Umul: return "Umul";
      case IIK_Puti: return "Puti";
   }

   return "Unknown";
}

IR IR_from_file(cstr file) {
   mcu_assert(file != nullptr, "file can't be null");

   IR self = {
      .Lexers = Vector_new(sizeof(Lexer)),
      .IrInstructions = Vector_new(sizeof(IrInstr))
   };

   return self;
}

void IR_delete(IR* self) {
   mcu_assert(self != nullptr, "self can't be null");

   Vector_free(&self->Lexers);
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
      println("%s:%u:%u: info: %s",
         lexer->file_path, loc.y, loc.x, IrInstrKind_to_cstr(instr->kind));
   }
}

