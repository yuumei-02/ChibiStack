// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include "lexer.h"
#include "ir.h"
#include "timing.h"

const cstr IrInstrKind_to_cstr(IrInstrKind self) {
   switch (self) {
      case IIK_PushInt:   return "PushInt";
      case IIK_PushUint:  return "PushUint";
      case IIK_PushAddr:  return "PushAddr";
      case IIK_Drop:      return "Drop";
      case IIK_Swap:      return "Swap";
      case IIK_Dup:       return "Dup";
      case IIK_Add:       return "Add";
      case IIK_Sub:       return "Sub";
      case IIK_Idiv:      return "Idiv";
      case IIK_Udiv:      return "Udiv";
      case IIK_Mul:       return "Mul";
      case IIK_Syscall0:  return "Syscall0";
      case IIK_Syscall1:  return "Syscall1";
      case IIK_Syscall2:  return "Syscall2";
      case IIK_Syscall3:  return "Syscall3";
      case IIK_Syscall4:  return "Syscall4";
      case IIK_Syscall5:  return "Syscall5";
      case IIK_Syscall6:  return "Syscall6";
      case IIK_ProcBegin: return "ProcBegin";
      case IIK_ProcEnd:   return "ProcEnd";
      case IIK_ProcCall:  return "ProcCall";
      case IIK_Puti:      return "Puti";
   }

   return "Unknown";
}

typedef struct {
   bool panic;
   bool failure;
   double* lexer_time;
} ParsingState;

static inline void enter_panic(ParsingState* state) {
   state->panic = true;
   state->failure = true;
}

static inline void exit_panic(ParsingState* state) {
   state->panic = false;
}

static inline u32 parse_code_block(IR* ir, Lexer* lexer, u32 lexer_i, ParsingState* state) {
   #define push_instr(instruction_kind) { \
      exit_panic(state); \
      instr.kind = instruction_kind; \
      Vector_push(&ir->IrInstructions, &instr); \
   }

   loop {
      Token token = Lexer_next(lexer, state->lexer_time);
      IrInstr instr = {
         .z = token.z,
         .lexer = lexer_i
      };

      switch (token.type) {
         case TT_End: exit_panic(state); return token.z;
      
         case TT_IntLiteral: {
            exit_panic(state);
            instr.kind = IIK_PushInt;
            instr.int_value = token.int_literal;
            Vector_push(&ir->IrInstructions, &instr);
         } break;

         case TT_StrLiteral: {
            exit_panic(state);
            instr.kind = IIK_PushUint;
            instr.uint_value = token.str_literal.length;
            Vector_push(&ir->IrInstructions, &instr);

            instr.kind = IIK_PushAddr;
            instr.uint_value = ir->string_literals.length;
            Vector_push(&ir->IrInstructions, &instr);

            Vector_push(&ir->string_literals, &token.str_literal);
         } break;

         case TT_Drop: push_instr(IIK_Drop) break;
         case TT_Swap: push_instr(IIK_Swap) break;
         case TT_Dup:  push_instr(IIK_Dup)  break;

         case TT_Add:  push_instr(IIK_Add)  break;
         case TT_Sub:  push_instr(IIK_Sub)  break;
         case TT_Idiv: push_instr(IIK_Idiv) break;
         case TT_Udiv: push_instr(IIK_Udiv) break;
         case TT_Mul:  push_instr(IIK_Mul)  break;

         case TT_Syscall0: push_instr(IIK_Syscall0) break;
         case TT_Syscall1: push_instr(IIK_Syscall1) break;
         case TT_Syscall2: push_instr(IIK_Syscall2) break;
         case TT_Syscall3: push_instr(IIK_Syscall3) break;
         case TT_Syscall4: push_instr(IIK_Syscall4) break;
         case TT_Syscall5: push_instr(IIK_Syscall5) break;
         case TT_Syscall6: push_instr(IIK_Syscall6) break;

         case TT_Puti: push_instr(IIK_Puti) break;

         // @Todo: Once we implement symbol tables and scopes,
         //        check whether or not the word exist and report an error if not.
         case TT_Word: {
            instr.word = token.str_view;
            push_instr(IIK_ProcCall)
         } break;

         case TT_Eof: {
            if (state->panic) return token.z;
            Loc loc = Lexer_loc_from_offset(lexer, token.z);
            eprintln("%s:%u:%u: error: Unexpected token \"Eof\"",
               lexer->file_path, loc.y, loc.x);
            enter_panic(state);
         } return token.z;

         default: {
            if (state->panic) break;
            Loc loc = Lexer_loc_from_offset(lexer, token.z);
            eprintln("%s:%u:%u: error: Unexpected token \"%s\"",
               lexer->file_path, loc.y, loc.x, TokenType_to_cstr(token.type));
            enter_panic(state);
         } break;
      }
   }

   #undef push_instr
}

static inline void parse_procedure(IR* ir, Lexer* lexer, u32 lexer_i, ParsingState* state) {
   Token token = Lexer_next(lexer, state->lexer_time);
   if (token.type != TT_Word) {
      Loc loc = Lexer_loc_from_offset(lexer, token.z);
      eprintln("%s:%u:%u: error: Unexpected token \"%s\", expected \"Word\"",
         lexer->file_path, loc.y, loc.x, TokenType_to_cstr(token.type));
      enter_panic(state);
      return;
   }

   StringView name = token.str_view;

   token = Lexer_next(lexer, state->lexer_time);
   if (token.type != TT_Begin) {
      Loc loc = Lexer_loc_from_offset(lexer, token.z);
      eprintln("%s:%u:%u: error: Unexpected token \"%s\", expected \"Begin\"",
         lexer->file_path, loc.y, loc.x, TokenType_to_cstr(token.type));
      enter_panic(state);
      return;
   }

   Vector_push_create(&ir->IrInstructions, ((IrInstr) {
      .kind = IIK_ProcBegin,
      .z = token.z,
      .word = name
   }));
   Vector_push_create(&ir->IrInstructions, ((IrInstr) {
      .kind = IIK_ProcEnd,
      .z = parse_code_block(ir, lexer, lexer_i, state)
   }));
}

IR IR_from_file(cstr file, bool* failure, double* ir_time, double* lexer_time) {
   mcu_assert(file != nullptr, "file can't be null");
   mcu_assert(failure != nullptr, "failure can't be null");
   mcu_assert(ir_time != nullptr, "ir_time can't be null");
   mcu_assert(lexer_time != nullptr, "lexer_time can't be null");

   struct timespec timer;
   clock_start(&timer);

   IR self = {
      .type_table = HashMap_new(Type)(),
      .Lexers = Vector_new(sizeof(Lexer)),
      .IrInstructions = Vector_new(sizeof(IrInstr)),
      .string_literals = Vector_new(sizeof(String))
   };

   Vector_push_create(&self.Lexers, (Lexer_new(file)));
   u32 lexer_i = (u32) (self.Lexers.length - 1);
   Lexer* lexer = Vector_get(&self.Lexers, self.Lexers.length - 1);
   ParsingState state = { .lexer_time = lexer_time };

   loop {
      Token token = Lexer_next(lexer, lexer_time);

      switch (token.type) {
         case TT_Proc: {
            state.panic = false;
            parse_procedure(&self, lexer, lexer_i, &state);
         } break;
      
         case TT_Eof: goto finish_parsing;
         default: {
            if (state.panic) break;
            Loc loc = Lexer_loc_from_offset(lexer, token.z);
            eprintln("%s:%u:%u: error: Unexpected token \"%s\", expected token \"proc\"",
               lexer->file_path, loc.y, loc.x, TokenType_to_cstr(token.type));
            state.panic = true;
            state.failure = true;
         }
      }
   }

finish_parsing:
   *failure = state.failure;
   *ir_time = clock_end(&timer);
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

   HashMap_free(Type)(&self->type_table);
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

