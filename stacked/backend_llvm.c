#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>

#include "lexer.h"
#include "lir.h"
#include "backend_llvm.h"

void llvm_hdr(FILE* outf) {
   fprintf(outf,
      "declare i32 @printf(i8*, ...)\n"
      "\n"
      "@format = private unnamed_addr constant [4 x i8] c\"%%ld\\0A\\00\"\n"
      "\n"
      "define i32 @main() {\n"
      "}");
}

cstr LIR_to_llvm_ir(LIR self, bool llvm_dump) {
   FILE* outf;
   if (!llvm_dump) {
      outf = fopen("./output.ll", "wb");
      if (outf == nullptr) {
         panic("[!] Failed to create file \"./output.ll\", reason: \"%s\"", strerror(errno));
      }
   } else {
      outf = stdout;
   }

   llvm_hdr(outf);

   if (!llvm_dump) {
      if (fclose(outf)) {
         eprintln("[!] Failed to close file \"./output.ll\", reason: \"%s\"", strerror(errno));
         eprintln("[!] Continuing");
      }
   }

   return "output.ll";
}
