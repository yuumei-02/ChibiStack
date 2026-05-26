#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/io.h>
#include <mcu/memory.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "lexer.h"
#include "lir.h"
#include "backend_llvm.h"

typedef enum {
   SR_Rax,
   SR_Rdi,
   SR_Rsi,
   SR_Rdx,
   SR_R10,
   SR_R8,
   SR_R9
} SyscallReg;

const cstr SyscallReg_to_cstr(SyscallReg self) {
   switch (self) {
      case SR_Rax: return "rax";
      case SR_Rdi: return "rdi";
      case SR_Rsi: return "rsi";
      case SR_Rdx: return "rdx";
      case SR_R10: return "r10";
      case SR_R8:  return "r8";
      case SR_R9:  return "r9";
   }
   return "Unknown";
}

void llvm_hdr(LIR* lir, FILE* outf) {
   fprintf(outf,
      "declare i32 @printf(i8*, ...)\n"
      "\n"
      "@format_puti = private unnamed_addr constant [5 x i8] c\"%%ld\\0A\\00\"\n"
      "@format_puts = private unnamed_addr constant [3 x i8] c\"%%s\\00\"\n\n");

   foreach (lir->StrLiterals, i) {
      String* str = Vector_get(&lir->StrLiterals, i);
      fprintf(outf,
         "@strlit_%zu = private unnamed_addr constant [%zu x i8] c\"%s\\00\"\n",
         i, str->length + 1, str->chars);
   }
      
   fprintf(outf,
      "define i32 @main() {\n"
      "entry:\n");
}

// @todo: allocate the stack upfront to allow for more llvm optimizations
void LIR_translate(LIR* self, FILE* outf) {
   u64 push_count   = 0;
   u64 val_count    = 0;
   u64 result_count = 0;
   u64 tmp_count    = 0;
   u64 label_count  = 0;
   u64 end_count    = 0;
   
   u64 stack_depth  = 0;
   u64 stack[MiB] = {0};

   bool puti_format_defined = false;
   bool puts_format_defined = false;

   foreach (self->Instructions, i) {
      Instruction* instr = Vector_get(&self->Instructions, i);

      switch (instr->type) {
         case IT_PushStr: {
            fprintf(outf, "   ; PushStr\n");
            fprintf(outf, "   %%push_%lu = alloca i64\n", push_count);
            stack[stack_depth++] = push_count++;

            String* str_lit = Vector_get(&self->StrLiterals, (usize) instr->args[0]);
            fprintf(outf, "   %%tmp_%lu = getelementptr [%zu x i8], ptr @strlit_%ld, i32 0, i32 0\n",
               tmp_count++, str_lit->length + 1, instr->args[0]);
            fprintf(outf, "   store ptr %%tmp_%lu, i64* %%push_%lu\n\n", tmp_count - 1, stack[stack_depth - 1]);
         } continue;
      
         case IT_Push: {
            fprintf(outf, "   ; Push\n");
            fprintf(outf, "   %%push_%lu = alloca i64\n", push_count);
            stack[stack_depth++] = push_count++;
            fprintf(outf, "   store i64 %ld, i64* %%push_%lu\n\n", instr->args[0], stack[stack_depth - 1]);
         } continue;

         case IT_Pop: {
            fprintf(outf, "   ; Pop\n");
            stack_depth--;
         } continue;

         case IT_Swap: {
            fprintf(outf, "   ; Swap\n");
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[stack_depth - 1]);
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[stack_depth - 2]);
            fprintf(outf, "   store i64 %%val_%lu, i64* %%push_%lu\n", val_count - 1, stack[stack_depth - 1]);
            fprintf(outf, "   store i64 %%val_%lu, i64* %%push_%lu\n", val_count - 2, stack[stack_depth - 2]);
         } continue;

         case IT_Dup: {
            fprintf(outf, "   ; Dup\n");
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n\n", val_count++, stack[stack_depth - 1]);
            fprintf(outf, "   %%push_%lu = alloca i64\n", push_count);
            stack[stack_depth++] = push_count++;
            fprintf(outf, "   store i64 %%val_%lu, i64* %%push_%lu\n\n", val_count - 1, stack[stack_depth - 1]);
         } continue;

         case IT_Label: {
            switch ((LabelType) instr->args[0]) {
               case LT_If: {
                  fprintf(outf, "   ; If label\n");
                  fprintf(outf, "if_label_%ld:\n", label_count++);
                  fprintf(outf, "   ");
               } continue;
            }
            
            panic("unreachable");
         } continue;

         case IT_Not: {
            fprintf(outf, "   ; Not\n");
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[--stack_depth]);
            fprintf(outf, "   %%result_%lu = icmp eq i64 %%val_%lu, 0\n", result_count++, val_count - 1);
            fprintf(outf, "   %%tmp_%lu = zext i1 %%result_%lu to i64\n", tmp_count++, result_count - 1);
            fprintf(outf, "   %%push_%lu = alloca i64", push_count);
            stack[stack_depth++] = push_count++;
            fprintf(outf, "   store i64 %%tmp_%lu, i64* %%push_%lu\n\n", tmp_count - 1, stack[stack_depth - 1]);
         } continue;

         case IT_Equ:     fprintf(outf, "   ; Equ\n");     goto arithmatic;
         case IT_NotEqu:  fprintf(outf, "   ; NotEqu\n");  goto arithmatic;
         case IT_Less:    fprintf(outf, "   ; Less\n");    goto arithmatic;
         case IT_More:    fprintf(outf, "   ; More\n");    goto arithmatic;
         case IT_LessEqu: fprintf(outf, "   ; LessEqu\n"); goto arithmatic;
         case IT_MoreEqu: fprintf(outf, "   ; MoreEqu\n"); goto arithmatic;
         case IT_Add:     fprintf(outf, "   ; Add\n");     goto arithmatic;
         case IT_Sub:     fprintf(outf, "   ; Sub\n");     goto arithmatic;
         case IT_Mul:     fprintf(outf, "   ; Mul\n");     goto arithmatic;
         case IT_Div: {
            fprintf(outf, "   ; Div\n");
         arithmatic:
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[--stack_depth]);
            fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[--stack_depth]);
            fprintf(outf, "   %%result_%lu = ", result_count++);

            switch (instr->type) {
               case IT_Add:     fprintf(outf, "add");      break;
               case IT_Sub:     fprintf(outf, "sub");      break;
               case IT_Mul:     fprintf(outf, "mul");      break;
               case IT_Div:     fprintf(outf, "sdiv");     break;
               case IT_Equ:     fprintf(outf, "icmp eq");  break;
               case IT_NotEqu:  fprintf(outf, "icmp ne");  break;
               case IT_Less:    fprintf(outf, "icmp slt"); break;
               case IT_More:    fprintf(outf, "icmp sgt"); break;
               case IT_LessEqu: fprintf(outf, "icmp sle"); break;
               case IT_MoreEqu: fprintf(outf, "icmp sge"); break;
               
               default: panic("unreachable");
            }
            
            fprintf(outf, " i64 %%val_%lu, %%val_%lu\n", val_count - 1, val_count - 2);
            fprintf(outf, "   %%push_%lu = alloca i64\n", push_count);
            stack[stack_depth++] = push_count++;

            switch (instr->type) {
               case IT_Equ:     [[fallthrough]];
               case IT_NotEqu:  [[fallthrough]];
               case IT_Less:    [[fallthrough]];
               case IT_More:    [[fallthrough]];
               case IT_LessEqu: [[fallthrough]];
               case IT_MoreEqu: {
                  fprintf(outf, "   %%tmp_%lu = zext i1 %%result_%lu to i64\n", tmp_count++, result_count - 1);
                  fprintf(outf, "   store i64 %%tmp_%lu, i64* %%push_%lu\n\n", tmp_count - 1, stack[stack_depth - 1]);
               } break;
               
               default: {
                  fprintf(outf, "   store i64 %%result_%lu, i64* %%push_%lu\n\n", result_count - 1, stack[stack_depth - 1]);
               }
            }
         } continue;

         case IT_Syscall: {
            fprintf(outf, "   ; Syscall\n");
            for (i64 i = 0; i < instr->args[0]; ++i) {
               fprintf(outf, "   %%val_%lu = load i64, i64* %%push_%lu\n", val_count++, stack[--stack_depth]);
            }

            fprintf(outf, "   %%result_%lu = call i64 asm sideeffect \"syscall\", \"={rax},", result_count++);
            SyscallReg reg = SR_Rax;
            for (i64 i = 0; i < instr->args[0]; ++i) {
               fprintf(outf, "{%s}", SyscallReg_to_cstr(reg++));
               if (i + 1 < instr->args[0]) {
                  fprintf(outf, ",");
               }
            }

            fprintf(outf, "\" (");
            for (i64 i = 0; i < instr->args[0]; ++i) {
               fprintf(outf, "i64 %%val_%lu", (val_count - instr->args[0]) + i);
               if (i + 1 < instr->args[0]) {
                  fprintf(outf, ", ");
               }
            }
            fprintf(outf, ")\n");
            fprintf(outf, "   %%push_%lu = alloca i64\n", push_count);
            stack[stack_depth++] = push_count++;
            fprintf(outf, "   store i64 %%result_%lu, i64* %%push_%lu\n\n", result_count - 1, stack[stack_depth - 1]);
         } continue;

         case IT_Puti: {
            fprintf(outf, "   ; Puti\n");
            if (!puti_format_defined) {
               fprintf(outf, "   %%puti_fmt = getelementptr [5 x i8], [5 x i8]* @format_puti, i64 0, i64 0\n");
               puti_format_defined = true;
            }
            fprintf(outf, "   %%result_%lu = load i64, i64* %%push_%lu\n", result_count++, stack[--stack_depth]);
            fprintf(outf, "   call i32 (i8*, ...) @printf(i8* %%puti_fmt, i64 %%result_%lu)\n\n", result_count - 1);
         } continue;

         case IT_Puts: {
            fprintf(outf, "   ; Puts\n");
            if (!puts_format_defined) {
               fprintf(outf, "   %%puts_fmt = getelementptr [3 x i8], [3 x i8]* @format_puts, i64 0, i64 0\n");
               puts_format_defined = true;
            }
            fprintf(outf, "   %%result_%lu = load i64, i64* %%push_%lu\n", result_count++, stack[--stack_depth]);
            fprintf(outf, "   call i32 (i8*, ...) @printf(i8* %%puts_fmt, i64 %%result_%lu)\n\n", result_count - 1);
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

   llvm_hdr(&self, outf);
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
