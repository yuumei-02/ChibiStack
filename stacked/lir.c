#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/io.h>

#include "lexer.h"
#include "lir.h"

const cstr InstrType_to_cstr(InstrType self) {
   switch (self) {
      case IT_Push: return "Push";
      case IT_Pop:  return "Pop";
      case IT_Swap: return "Swap";
      case IT_Dup:  return "Dup";

      case IT_Add:   return "Add";
      case IT_Sub:   return "Sub";
      case IT_Mul:   return "Mul";
      case IT_Div:   return "Div";

      case IT_Puti:  return "Puti";
   }

   return "Unknown";
}

LIR LIR_from_lexer(Lexer* lexer) {
   mcu_assert(lexer != nullptr, "lexer can't be null");

   LIR self = Vector_new(sizeof(Instruction));

   loop {
      Token token = Lexer_next(lexer);

      Instruction instr = {
         .origin = {
            .x = token.x,
            .y = token.y,
            .length = token.length,
            .path = lexer->file.path
         }
      };
      
      switch (token.type) { 
         case TT_IntLiteral: {
            instr.type = IT_Push;
            instr.arg1 = token.int_literal;
            Vector_push(&self, &instr);
         } continue;

         case TT_Drop: instr.type = IT_Pop;  Vector_push(&self, &instr); continue;
         case TT_Swap: instr.type = IT_Swap; Vector_push(&self, &instr); continue;
         case TT_Dup:  instr.type = IT_Dup;  Vector_push(&self, &instr); continue;

         case TT_Add: instr.type = IT_Add; Vector_push(&self, &instr); continue;
         case TT_Sub: instr.type = IT_Sub; Vector_push(&self, &instr); continue;
         case TT_Mul: instr.type = IT_Mul; Vector_push(&self, &instr); continue;
         case TT_Div: instr.type = IT_Div; Vector_push(&self, &instr); continue;

         case TT_Puti: instr.type = IT_Puti; Vector_push(&self, &instr); continue;

         case TT_Identifier: mcu_todo("not yet implemented");

         case TT_Eof: {
            return self;
         }
      }

      panic("unreachable");
   }
}

void LIR_delete(nullable LIR* self) {
   if (self == nullptr) return;
   Vector_free(self);
}

void LIR_display(LIR* self) {
   foreach ((*self), i) {
      Instruction* instr = Vector_get(self, i);
      printf("%s:%zu:%zu:%zu: %s",
         instr->origin.path,
         instr->origin.y,
         instr->origin.x,
         instr->origin.length,
         InstrType_to_cstr(instr->type));

      switch (instr->type) {
         case IT_Push: {
            println(" (%ld)", instr->arg1);
         } break;

         default: {
            printf("\n");
         }
      }
   }
}

