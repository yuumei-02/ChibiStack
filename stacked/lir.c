#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/io.h>

#include "lexer.h"
#include "lir.h"

const cstr InstrType_to_cstr(InstrType self) {
   switch (self) {
      case IT_PushStr: return "PushStr";
      case IT_Push:    return "Push";
      case IT_Pop:     return "Pop";
      case IT_Swap:    return "Swap";
      case IT_Dup:     return "Dup";
      case IT_Label:   return "Label";
      case IT_CJmp:    return "CJmp";
      case IT_Add:     return "Add";
      case IT_Sub:     return "Sub";
      case IT_Mul:     return "Mul";
      case IT_Div:     return "Div";
      case IT_Equ:     return "Equ";
      case IT_NotEqu:  return "NotEqu";
      case IT_Less:    return "Less";
      case IT_More:    return "More";
      case IT_LessEqu: return "LessEqu";
      case IT_MoreEqu: return "MoreEqu";
      case IT_Not:     return "Not";
      case IT_Syscall: return "Syscall";
      case IT_Puti:    return "Puti";
      case IT_Puts:    return "Puts";
   }

   return "Unknown";
}

const cstr LabelType_to_cstr(LabelType self) {
   switch (self) {
      case LT_If:   return "if";
      case LT_End:  return "end";
   }
   return "Unknown";
}

void LIR_path_labels(nullable LIR* self) {
   if (self == nullptr) return;

   Vector label_stack = Vector_new(sizeof(usize));
   
   foreach (self->Instructions, i) {
      Instruction* instr = Vector_get(&self->Instructions, i);

      switch (instr->type) {
         case IT_CJmp: [[fallthrough]];
         case IT_Label: {
            Vector_push(&label_stack, &i);
         } break;

         default: break;
      }
   }

   usize end_ip = 0;

   for (i64 i = label_stack.length - 1; i >= 0; --i) {
      usize ip = *(usize*) Vector_get(&label_stack, (usize) i);
      Instruction* instr = Vector_get(&self->Instructions, ip);

      switch (instr->type) {
         case IT_CJmp: {
            instr->args[0] = end_ip;
         } continue;
         
         case IT_Label: {
            switch ((LabelType) instr->args[0]) {
               case LT_If: continue;
               case LT_End: {
                  end_ip = ip;
               } continue;
            }
         
            panic("unreachable");
         }
         
         default: panic("unreachable");
      }
   }

   Vector_free(&label_stack);
}

LIR LIR_from_lexer(Lexer* lexer) {
   mcu_assert(lexer != nullptr, "lexer can't be null");

   LIR self = {
      .Instructions = Vector_new(sizeof(Instruction)),
      .StrLiterals = Vector_new(sizeof(String))
   };

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
            instr.args[0] = token.int_literal;
            Vector_push(&self.Instructions, &instr);
         } continue;

         case TT_StrLiteral: {
            instr.type = IT_Push;
            instr.args[0] = (i64) token.length;
            Vector_push(&self.Instructions, &instr);

            instr.type = IT_PushStr;
            instr.args[0] = self.StrLiterals.length;
            Vector_push(&self.Instructions, &instr);
            Vector_push(&self.StrLiterals, &token.str_literal);
         } continue;

         case TT_Drop: instr.type = IT_Pop;  Vector_push(&self.Instructions, &instr); continue;
         case TT_Swap: instr.type = IT_Swap; Vector_push(&self.Instructions, &instr); continue;
         case TT_Dup:  instr.type = IT_Dup;  Vector_push(&self.Instructions, &instr); continue;

         case TT_If:   instr.args[0] = LT_If; goto labels;
         case TT_End: {
            instr.args[0] = LT_End;
         labels:
            instr.type = IT_Label;
            Vector_push(&self.Instructions, &instr);
         } continue;

         case TT_Do: {
            instr.type = IT_CJmp;
            Vector_push(&self.Instructions, &instr);
         } continue;

         case TT_Add: instr.type = IT_Add; Vector_push(&self.Instructions, &instr); continue;
         case TT_Sub: instr.type = IT_Sub; Vector_push(&self.Instructions, &instr); continue;
         case TT_Mul: instr.type = IT_Mul; Vector_push(&self.Instructions, &instr); continue;
         case TT_Div: instr.type = IT_Div; Vector_push(&self.Instructions, &instr); continue;

         case TT_Equals:    instr.type = IT_Equ;     Vector_push(&self.Instructions, &instr); continue;
         case TT_NotEquals: instr.type = IT_NotEqu;  Vector_push(&self.Instructions, &instr); continue;
         case TT_Less:      instr.type = IT_Less;    Vector_push(&self.Instructions, &instr); continue;
         case TT_More:      instr.type = IT_More;    Vector_push(&self.Instructions, &instr); continue;
         case TT_LessEqu:   instr.type = IT_LessEqu; Vector_push(&self.Instructions, &instr); continue;
         case TT_MoreEqu:   instr.type = IT_MoreEqu; Vector_push(&self.Instructions, &instr); continue;
         case TT_Not:       instr.type = IT_Not;     Vector_push(&self.Instructions, &instr); continue;

         case TT_Syscall1: instr.args[0] = 1; goto syscall;
         case TT_Syscall2: instr.args[0] = 2; goto syscall;
         case TT_Syscall3: instr.args[0] = 3; goto syscall;
         case TT_Syscall4: instr.args[0] = 4; goto syscall;
         case TT_Syscall5: instr.args[0] = 5; goto syscall;
         case TT_Syscall6: {
            instr.args[0] = 6;
         syscall:
            instr.type = IT_Syscall;
            Vector_push(&self.Instructions, &instr);
         } continue;

         case TT_Puti: instr.type = IT_Puti; Vector_push(&self.Instructions, &instr); continue;
         case TT_Puts: instr.type = IT_Puts; Vector_push(&self.Instructions, &instr); continue;

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
   Vector_free(&self->Instructions);

   foreach (self->StrLiterals, i) {
      String* str = Vector_get(&self->StrLiterals, i);
      String_free(str);
   }
   
   Vector_free(&self->StrLiterals);
}

void LIR_display(LIR* self) {
   foreach (self->Instructions, i) {
      Instruction* instr = Vector_get(&self->Instructions, i);
      printf("%s:%zu:%zu:%zu: %s",
         instr->origin.path,
         instr->origin.y,
         instr->origin.x,
         instr->origin.length,
         InstrType_to_cstr(instr->type));

      switch (instr->type) {
         case IT_Push: {
            println(" (%ld)", instr->args[0]);
         } break;

         case IT_Label: {
            println(" (%s)", LabelType_to_cstr(instr->args[0]));
         } break;

         case IT_CJmp: {
            println(" (%ld)", instr->args[0]);
         } break;

         default: {
            printf("\n");
         }
      }
   }
}

