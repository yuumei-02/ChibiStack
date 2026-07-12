// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "flags.h"
#include "lexer.h"
#include "ir.h"

void token_dump(cstr file) {
   Lexer lexer = Lexer_new(file);
   Token token;
   do {
      token = Lexer_next(&lexer);
      Token_dump(token, &lexer);
   } while (token.type != TT_Eof);
   
   Lexer_delete(&lexer);
}

void compile(cstr file, CompileFlags flags) {
   mcu_assert(file != nullptr, "file can't be null");

   if (flags.token_dump) {
      token_dump(file);
      return;
   }

   IR ir = IR_from_file(file);
   if (flags.ir_dump) {
      IR_dump(&ir);
      IR_delete(&ir);
      return;
   }

   IR_delete(&ir);
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
         else {
            Vector_push(&files, &argv[i]);
         }
      }
   }

   foreach (files, i) {
      cstr* file = Vector_get(&files, i);
      compile(*file, flags);
   }

   Vector_free(&files);
   return 0;
}

