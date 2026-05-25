#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "lexer.h"
#include "lir.h"
#include "backend_llvm.h"

void llvm_hdr(FILE* outf) {
   fprintf(outf,
      "declare i32 @printf(i8*, ...)\n"
      "\n"
      "@format = private unnamed_addr constant [5 x i8] c\"%%ld\\0A\\00\"\n"
      "\n"
      "define i32 @main() {\n");
}

void LIR_translate(LIR* self, FILE* outf) {
   u64 push_count   = 0;
   u64 val_count    = 0;
   u64 result_count = 0;
   bool format_defined = false;

   foreach ((*self), i) {
      Instruction* instr = Vector_get(self, i);

      switch (instr->type) {
         case IT_Push: {
            fprintf(outf, "   ; Push\n");
            fprintf(outf, "   %%push_%zu = alloca i64\n", push_count++);
            fprintf(outf, "   store i64 %ld, i64* %%push_%zu\n\n", instr->arg1, push_count - 1);
         } continue;

         case IT_Add: {
            fprintf(outf, "   ; Add\n");
            fprintf(outf, "   %%val_%zu = load i64, i64* %%push_%zu\n", val_count++, push_count - 1);
            fprintf(outf, "   %%val_%zu = load i64, i64* %%push_%zu\n", val_count++, push_count - 2);
            fprintf(outf, "   %%result_%zu = add i64 %%val_%zu, %%val_%zu\n", result_count++, val_count - 1, val_count - 2);
            fprintf(outf, "   %%push_%zu = alloca i64\n", push_count++);
            fprintf(outf, "   store i64 %%result_%zu, i64* %%push_%zu\n\n", result_count - 1, push_count - 1);
         } continue;

         case IT_Sub: mcu_todo("not yet implemented");
         case IT_Mul: mcu_todo("not yet implemented");
         case IT_Div: mcu_todo("not yet implemented");

         case IT_Print: {
            fprintf(outf, "   ; Print\n");
            if (!format_defined) {
               fprintf(outf, "   %%fmt = getelementptr [5 x i8], [5 x i8]* @format, i64 0, i64 0\n");
               format_defined = true;
            }
            fprintf(outf, "   %%result_%zu = load i64, i64* %%push_%zu\n", result_count++, push_count - 1);
            fprintf(outf, "   call i32 (i8*, ...) @printf(i8* %%fmt, i64 %%result_%zu)\n\n", result_count - 1);
         } continue;
      }

      panic("unreachable");
   }
}

String path_trim_file_path(cstr path) {
   String trimmed = String_new();

   path += strlen(path) - 1;

   bool contains_extension = false;
   while (*path != '\0') {
      if (*path == '.') contains_extension = true;
      if (*path == '/') break;
      String_append_back(&trimmed, *path);
      path--;
   }

   if (contains_extension) {
      while (String_pop(&trimmed) != '.');
   }

   return trimmed;
}

bool execute_cmd(cstr command) {
   mcu_assert(command != nullptr, "command can't be null");

   errno = 0;
   println("[i] %s", command);
   i32 result = system(command);

   if (errno != 0 || result != 0)
      return true;
   return false;
}

i32 LIR_to_llvm_ir(LIR self, bool llvm_dump, cstr out_file) {
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
   LIR_translate(&self, outf);
   fprintf(outf,
      "   ret i32 0\n"
      "}");

   if (llvm_dump) {
      return 0;
   }

   if (fclose(outf)) {
      eprintln("[!] Failed to close file \"./output.ll\", reason: \"%s\"", strerror(errno));
      eprintln("[!] Continuing");
   }

   i32 result = 0;
   if (execute_cmd("llc -filetype=obj ./output.ll -o output.o")) result = 1;
   
   String out_name = path_trim_file_path(out_file);
   String ld_command = String_new();
   String_appendf(&ld_command, "ld"
      " -o %s"
      " -dynamic-linker /lib64/ld-linux-x86-64.so.2"
      " /usr/lib/crt1.o"
      " /usr/lib/crti.o"
      " /usr/lib/crtn.o"
      " ./output.o"
      " -lc"
      " -L/usr/lib/x86_64-pc-linux-gnu"
      " -L/usr/lib",
      out_name.chars);

   if (execute_cmd(ld_command.chars)) result = 1;
   if (execute_cmd("rm output.ll output.o")) result = 1;

   String_free(&ld_command);
   String_free(&out_name);

   return result;
}
