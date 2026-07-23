// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>
#include <mcu/io.h>

#include "timing.h"
#include "lexer.h"
#include "ir.h"
#include "reporter.h"
#include "utils.h"

extern TypeInfo int_type;
extern TypeInfo uint_type;
extern TypeInfo ptr_type;
extern u32 next_type_id;

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

   Vector type_stack = Vector_new(sizeof(TypeInfo));
   foreach (ir->IrInstructions, i) {
      IrInstr* instr = Vector_get(&ir->IrInstructions, i);

      switch (instr->kind) {
         case IIK_PushInt: {
            Vector_push_create(&type_stack, ((TypeInfo) {
               .lexer_i = instr->lexer,
               .id = int_type.id,
               .offset = instr->z,
               .name = int_type.name
            }));
         } continue;

         case IIK_PushUint: {
            Vector_push_create(&type_stack, ((TypeInfo) {
               .lexer_i = instr->lexer,
               .id = uint_type.id,
               .offset = instr->z,
               .name = uint_type.name
            }));
         } continue;

         case IIK_PushAddr: {
            Vector_push_create(&type_stack, ((TypeInfo) {
               .lexer_i = instr->lexer,
               .id = ptr_type.id,
               .offset = instr->z,
               .name = ptr_type.name
            }));
         } continue;

         case IIK_Drop: {
            minimum_required_stack_size(1);
            Vector_pop(&type_stack);
         } continue;

         // @Todo: Should TypeInfo instances that are pushed to the type_stack by stack operations
         //        update those TypeInfo instances to point to the stack operation in source or to
         //        their origin in source?
         //        - Yuumei-02, 13-07-2026 15:26
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
               Lexer* lexi = get_lexer_from_lexer_id(&ir->Lexers, instr->lexer);
               report_incompatible_binop_types(instr->kind, a.name, b.name,
                  lexi, instr->z);
               goto failure;
            }

            a.lexer_i = instr->lexer;
            a.offset  = instr->z;
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
                     get_lexer_from_lexer_id(&ir->Lexers, a.lexer_i), a.offset);
               }

               if (b.id != expected_type.id) {
                  report_unexpected_type(b.name, expected_type.name,
                     get_lexer_from_lexer_id(&ir->Lexers, b.lexer_i), b.offset);
               }

               goto failure;
            }

            a.lexer_i = instr->lexer;
            a.offset  = instr->z;
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

            Vector_push_create(&type_stack, ((TypeInfo) {
               .id = int_type.id,
               .name = int_type.name,
               .lexer_i = instr->lexer,
               .offset = instr->z
            }));
         } continue;

         case IIK_ProcBegin: {
            starting_stack_size = type_stack.length;

            char tmp = sv_tmp_nullify(instr->word);
            Symbol* symbol = HashMap_get(Symbol)(&ir->symbol_table, instr->word.chars);
            sv_tmp_restore(instr->word, tmp);
            mcu_assert(symbol != nullptr, "The compilation should've already terminated in this case");
            mcu_assert(symbol->kind == SK_Proc, "Unreachable");

            Type* type = HashMap_get(Type)(&ir->type_table, symbol->proc.signature.chars);
            mcu_assert(type != nullptr, "Unreachable");
            mcu_assert(type->kind == TK_Proc, "Unreachable");

            foreach (type->proc.parameter_types, i) {
               TypeInfo* parameter_type = Vector_get(&type->proc.parameter_types, i);
               Vector_push(&type_stack, parameter_type);
            }
         } continue;

         case IIK_ProcEnd: {
            char tmp = sv_tmp_nullify(instr->word);
            Symbol* symbol = HashMap_get(Symbol)(&ir->symbol_table, instr->word.chars);
            sv_tmp_restore(instr->word, tmp);

            Type* type = HashMap_get(Type)(&ir->type_table, symbol->proc.signature.chars);

            if (type_stack.length != starting_stack_size + type->proc.return_types.length) {
               report_unhandled_stack_items((i32) type_stack.length - (i32) starting_stack_size,
                  get_lexer_from_lexer_id(&ir->Lexers, instr->lexer), instr->z);
               goto failure;
            }

            type_stack.length -= type->proc.return_types.length;
         } continue;

         case IIK_ProcCall: {
            char tmp = StringView_tmp_nullify(instr->word);
            Symbol* proc_sym = HashMap_get(Symbol)(&ir->symbol_table, instr->word.chars);
            StringView_tmp_restore(instr->word, tmp);
            Type* type = HashMap_get(Type)(&ir->type_table, proc_sym->proc.signature.chars);

            foreach (type->proc.parameter_types, i) {
               Vector_pop(&type_stack);
            }

            foreach (type->proc.return_types, i) {
               TypeInfo return_type = *(TypeInfo*) Vector_get(&type->proc.return_types, i);
               return_type.lexer_i = instr->lexer;
               return_type.offset = instr->lexer;
            
               Vector_push(&type_stack, &return_type);
            }
         } continue;

         case IIK_Puti: {
            minimum_required_stack_size(1);

            TypeInfo a = *(TypeInfo*) Vector_pop(&type_stack);
            if (a.id != int_type.id) {
               report_unexpected_type(a.name, int_type.name,
                  get_lexer_from_lexer_id(&ir->Lexers, a.lexer_i), a.offset);
               goto failure;
            }
         } continue;

         case IIK_InvalidSymbol: continue;
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

