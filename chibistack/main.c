// Copyright (c) 2026 Yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "flags.h"
#include "lexer.h"
#include "ir.h"
#include "semantic_analyzer.h"
#include "x86_codegen.h"

double token_dump(cstr file) {
   Lexer lexer = Lexer_new(file, nullptr);
   Token token;
   double lexer_time = 0.0f;
   
   do {
      token = Lexer_next(&lexer, &lexer_time);
      Token_dump(token, &lexer);
   } while (token.type != TT_Eof);
   
   Lexer_delete(&lexer);
   return lexer_time;
}

i32 compile(cstr file, CompileFlags flags) {
   mcu_assert(file != nullptr, "file can't be null");

   bool failure;
   double lexer_time             = 0.0f;
   double ir_time                = 0.0f;
   double semantic_analysis_time = 0.0f;
   double code_gen_time          = 0.0f;
   double linker_time            = 0.0f;
   
   if (flags.token_dump) {
      lexer_time = token_dump(file);
      failure = false;
      goto statistics;
   }

   IR ir = IR_from_file(file, &failure, &ir_time, &lexer_time);
   if (failure) goto failure;

   if (validate_program(&ir, &semantic_analysis_time))
      goto failure;
   
   if (flags.ir_dump) {
      IR_dump(&ir);
      goto success;
   }

   if (nasm_from_ir(&ir, flags.asm_dump, &code_gen_time, &linker_time)) goto failure;
   
success:
   failure = false;
   goto cleanup;
failure:
   failure = true;
cleanup:
   IR_delete(&ir);
statistics:
   u32 tokens_parsed = get_tokens_parsed();
   println("\nCompilation statistics");
   println("--------------------------------------");
   println("Target : x86_64 Linux");
   println("--------------------------------------");
   println("Tokens            : %.2lfm Tokens/s (%u tokens)", ((double) tokens_parsed / lexer_time) / 1'000'000.0f, tokens_parsed);
   println("Lexer             : %.6lf seconds", lexer_time);
   println("Parser & IR       : %.6lf seconds", ir_time - lexer_time);
   println("Semantic analysis : %.6lf seconds", semantic_analysis_time);
   println("Code generation   : %.6lf seconds", code_gen_time);
   println("Nasm & Linker     : %.6lf seconds", linker_time);
   println("--------------------------------------");
   println("Total time        : %.6lf seconds", ir_time + code_gen_time + linker_time);
   println("--------------------------------------");
   println(failure == false
      ? "Compilation succeeded\n"
      : "Compilation failure\n");
   
   return failure;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("[!] Missing input file(s)");
      return 1;
   }

   CompileFlags flags = CompileFlags_default();
   Vector files = Vector_new(sizeof(cstr));

   for (i32 i = 1; i < argc; ++i) {
      cstr_match(argv[i]) {
        ncstreq("--token-dump") flags.token_dump = true;
         cstreq("--ir-dump")    flags.ir_dump    = true;
         cstreq("--asm-dump")   flags.asm_dump   = true;
         else {
            Vector_push(&files, &argv[i]);
         }
      }
   }

   foreach (files, i) {
      cstr* file = Vector_get(&files, i);
      if (compile(*file, flags)) {
         return 1;
      }
   }

   Vector_free(&files);
   return 0;
}

