#include <mcu/core.h>
#include <mcu/handlers.h>
#include <mcu/io.h>

#include "lexer.h"
#include "lir.h"

i32 compile(cstr file_path) {
   Lexer lexer = Lexer_new(file_path);

   /* Token token; */
   /* do { */
   /*    token = Lexer_next(&lexer); */
   /*    Token_display(lexer.file.path, token); */
   /*    Token_free(token); */
   /* } while (token.type != TT_Eof); */

   LIR lir = LIR_from_lexer(&lexer);
   LIR_display(&lir);

   Lexer_delete(&lexer);
   return 0;
}

i32 main(i32 argc, cstr argv[]) {
   if (argc < 2) {
      eprintln("[!] Missing input files");
      return 1;
   }

   for (i32 i = 1; i < argc; ++i) {
      if (compile(argv[i])) return 1;
   }

   return 0;
}

