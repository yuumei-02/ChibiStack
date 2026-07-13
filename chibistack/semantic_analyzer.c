// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>
#include <mcu/io.h>

#include "timing.h"
#include "lexer.h"
#include "ir.h"

HashMap_impl(Type)

typedef struct {
   u32 id;
   cstr name;
} TypeInfo;

static TypeInfo int_type  = { .id = 0, .name = "int"  };
static TypeInfo uint_type = { .id = 1, .name = "uint" };
static TypeInfo ptr_type  = { .id = 2, .name = "ptr"  };
static u32 next_type_id = 3;

static void create_build_in_types(IR* ir) {
   HashMap_put(Type)(&ir->type_table, int_type.name,  (Type) { .type_id = int_type.id });
   HashMap_put(Type)(&ir->type_table, uint_type.name, (Type) { .type_id = uint_type.id });
   HashMap_put(Type)(&ir->type_table, ptr_type.name,  (Type) { .type_id = ptr_type.id });
}

void report_not_enough_stack_elements(const cstr for_instr, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Not enough items on the stack for operation \"%s\"",
      lexer->file_path, loc.y, loc.x, for_instr);
}

void report_unexpected_type(const cstr got, const cstr expected, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Expected Type \"%s\" got \"%s\"",
      lexer->file_path, loc.y, loc.x, expected, got);
}

void report_incompatible_binop_types(IrInstrKind operator, const cstr left, const cstr right, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: Type \"%s\" and \"%s\" are not compatible for operator \"%s\"",
      lexer->file_path, loc.y, loc.x,
      left, right, IrInstrKind_to_cstr(operator));
}

void report_unhandled_stack_items(i32 amount, Lexer* lexer, u32 offset) {
   Loc loc = Lexer_loc_from_offset(lexer, offset);
   eprintln("%s:%u:%u: error: %d item(s) of unhandled data on the stack",
      lexer->file_path, loc.y, loc.x, amount);
}

Lexer* get_lexer_from_lexer_id(Vector* Lexers, u16 lexer_i) {
   Lexer* lexer = Vector_get(Lexers, (usize) lexer_i);
   mcu_assert(lexer != nullptr, "lexer can't be null");
   return lexer;
}

bool validate_program(IR* ir, double* semantic_analysis_time) {
   mcu_assert(ir != nullptr, "ir can't be null");
   mcu_assert(semantic_analysis_time != nullptr, "semantic_analysis_time can't be null");

   struct timespec timer;
   clock_start(&timer);

   bool failure;

   // @Todo: Currently, whenever there is a type error detected. Compilation immediately gets aborted.
   //        This is fine for now but in the future we may want to catch as many semantic errors as possible
   //        before aborting compilation.
   //        One way we could and probably should do this, is by aborting checks on a per procedure basis.
   //        Whenever the stack is semantically wrong, we only abort analysis for that one procedure so that
   //        we can keep on analysing the rest before we ultimately abort compilation.
   //        - Yuumei-02, 13-07-2026 14:22

   #define minimum_required_stack_size(required_size) do { \
      if (type_stack.length < required_size) { \
         report_not_enough_stack_elements(IrInstrKind_to_cstr(instr->kind), \
            get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z); \
         goto failure; \
      } \
   } while (0)

   u32 starting_stack_size = 0;

   create_build_in_types(ir);
   Vector type_stack = Vector_new(sizeof(TypeInfo));
   foreach (ir->IrInstructions, i) {
      IrInstr* instr = Vector_get(&ir->IrInstructions, i);

      switch (instr->kind) {
         case IIK_PushInt:  Vector_push(&type_stack, &int_type);  continue;
         case IIK_PushUint: Vector_push(&type_stack, &uint_type); continue;
         case IIK_PushAddr: Vector_push(&type_stack, &ptr_type);  continue;

         case IIK_Drop: {
            minimum_required_stack_size(1);
            Vector_pop(&type_stack);
         } continue;

         case IIK_Swap: {
            minimum_required_stack_size(2);
         
            TypeInfo b = *(TypeInfo*) Vector_pop(&type_stack);
            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);

            Vector_push(&type_stack, &b);
            Vector_push(&type_stack, &a);
         } continue;

         case IIK_Dup: {
            minimum_required_stack_size(1);

            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);
            Vector_push(&type_stack, &a);
            Vector_push(&type_stack, &a);
         } continue;

         case IIK_Add: [[fallthrough]];
         case IIK_Sub: [[fallthrough]];
         case IIK_Mul: {
            minimum_required_stack_size(2);

            TypeInfo b = *(TypeInfo*) Vector_pop(&type_stack);
            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);

            if (a.id != b.id) {
               report_incompatible_binop_types(instr->kind, a.name, b.name,
                  get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               goto failure;
            }

            Vector_push(&type_stack, &a);
         } continue;

         case IIK_Udiv: [[fallthrough]];
         case IIK_Idiv: {
            minimum_required_stack_size(2);

            TypeInfo b = *(TypeInfo*) Vector_pop(&type_stack);
            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);

            TypeInfo expected_type = instr->kind == IIK_Idiv
               ? int_type
               : uint_type;

            if (a.id != expected_type.id || b.id != expected_type.id) {
               if (a.id != expected_type.id) {
                  report_unexpected_type(a.name, expected_type.name,
                     get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               }

               if (b.id != expected_type.id) {
                  report_unexpected_type(b.name, expected_type.name,
                     get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               }

               goto failure;
            }

            Vector_push(&type_stack, &a);
         } continue;

         case IIK_Syscall0: [[fallthrough]];
         case IIK_Syscall1: [[fallthrough]];
         case IIK_Syscall2: [[fallthrough]];
         case IIK_Syscall3: [[fallthrough]];
         case IIK_Syscall4: [[fallthrough]];
         case IIK_Syscall5: [[fallthrough]];
         case IIK_Syscall6: {
            u32 required_stack_size;
         
            switch (instr->kind) {
               case IIK_Syscall0: required_stack_size = 1; break;
               case IIK_Syscall1: required_stack_size = 2; break;
               case IIK_Syscall2: required_stack_size = 3; break;
               case IIK_Syscall3: required_stack_size = 4; break;
               case IIK_Syscall4: required_stack_size = 5; break;
               case IIK_Syscall5: required_stack_size = 6; break;
               case IIK_Syscall6: required_stack_size = 7; break;

               default: panic("unreachable");
            }
         
            minimum_required_stack_size(required_stack_size);

            for (u32 i = 0; i < required_stack_size; ++i)
               Vector_pop(&type_stack);
            
            Vector_push(&type_stack, &int_type);
         } continue;

         case IIK_ProcBegin: {
            starting_stack_size = type_stack.length;
         } continue;

         case IIK_ProcEnd: {
            if (type_stack.length != starting_stack_size) {
               report_unhandled_stack_items((i32) type_stack.length - (i32) starting_stack_size,
                  get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               goto failure;
            }
         } continue;

         // @Todo: Check if the procedure exist in scope.
         case IIK_ProcCall: continue;

         case IIK_Puti: {
            minimum_required_stack_size(1);

            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);
            if (a.id != int_type.id) {
               report_unexpected_type(a.name, int_type.name,
                  get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               goto failure;
            }
         } continue;
      }

      panic("unreachable");
   }

   failure = false;
   goto cleanup;

failure:
   failure = true;

cleanup:
   Vector_free(&type_stack);
   *semantic_analysis_time = clock_end(&timer);
   return failure;
}

