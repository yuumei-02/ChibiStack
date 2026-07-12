// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "lexer.h"

i32 compile(cstr file) {
   mcu_assert(file != nullptr, "file can't be null");

   Lexer lexer = Lexer_new(file);
   Token token;
   do {
      token = Lexer_next(&lexer);
      Token_dump(token, &lexer);
   } while (token.type != TT_Eof);
   
   Lexer_delete(&lexer);
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("[!] Missing input file(s)");
      return 1;
   }

   for (i32 i = 1; i < argc; ++i) {
      if (compile(argv[i])) {
         return 1;
      }
   }

   return 0;
}

