#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "flags.h"
#include "lexer.h"
#include "lir.h"
#include "backend_llvm.h"

void token_dump(Lexer* lexer) {
   Token token;
   do {
      token = Lexer_next(lexer);
      Token_display(lexer->file.path, token);
      Token_free(token);
   } while (token.type != TT_Eof);
}

i32 compile(cstr file_path, CompileFlags flags) {
   Lexer lexer = Lexer_new(file_path);
   if (flags.token_dump) {
      token_dump(&lexer);
      return 0;
   }

   LIR lir = LIR_from_lexer(&lexer);
   if (flags.lir_dump) {
      LIR_display(&lir);
      return 0;
   }

   LIR_to_llvm_ir(lir, flags.llvm_dump, file_path);

   Lexer_delete(&lexer);
   LIR_delete(&lir);
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("[!] Missing input files");
      return 1;
   }

   CompileFlags flags = CompileFlags_default();
   Vector files = Vector_new(sizeof(cstr));
   
   for (i32 i = 1; i < argc; ++i) {
      cstr_match(argv[i]) {
         ncstreq("--token-dump") flags.token_dump = true;
         cstreq("--lir-dump")    flags.lir_dump   = true;
         cstreq("--llvm-dump")   flags.llvm_dump  = true;
         else {
            Vector_push(&files, &argv[i]);
         }
      }
   }

   foreach (files, i) {
      cstr* file = Vector_get(&files, i);
      if (compile(*file, flags)) {
         Vector_free(&files);
         return 1;
      }
   }

   Vector_free(&files);
   return 0;
}

