// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#define _POSIX_C_SOURCE 200809L
#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/containers.h>

#include <string.h>
#include <errno.h>

#include "lexer.h"
#include "ir.h"
#include "x86_codegen.h"

#define execute_command(format, ...) \
   execute_command_impl(true, false, format __VA_OPT__(,) __VA_ARGS__)

// @Note: Taken from my Vlodinnye make library
// @Link: https://github.com/yuumei-02/Vlodinnye-make
static i32 execute_command_impl(bool echo, bool suppress, const cstr format, ...) {
   va_list args;
   va_start(args, format);
   String command = String_new();
   String_appendfv(&command, format, args);
   va_end(args);
   
   char output_buffer[1024*4];
   String_append_cstr(&command, " 2>&1");

   if (echo) println("[i] %s", command.chars);

   FILE* childp = popen(command.chars, "r");
   String_free(&command);
   if (childp == nullptr) {
      eprintln("[!] Failed to execute command, reason: \"%s\"", strerror(errno));
      return -1;
   }

   while (fgets(output_buffer, sizeof(output_buffer), childp) != null) {
      if (!suppress) printf("%s", output_buffer);
   }

   if (ferror(childp)) {
      eprintln("[!] Failed to read from pipe, reason: \"%s\"", strerror(errno));
      pclose(childp);
      return -1;
   }

   i32 result = pclose(childp);
   if (result == -1) {
      eprintln("[!] Failed to close pipe, reason: \"%s\"", strerror(errno));
      eprintln("[!] Proceeding...");
   }

   return result;
}

static FILE* get_file_handle(bool asm_dump) {
   if (asm_dump) return stdout;

   FILE* handle = fopen("./output.asm", "w");
   if (handle == nullptr)
      panic("[!] Failed to open file \"./output.asm\", reason: \"%s\"", strerror(errno));

   return handle;
}

static void close_file_handle(FILE* handle, bool asm_dump) {
   if (asm_dump) return;
   if (fclose(handle)) {
      eprintln("[!] Failed to close file \"./output.asm\", reason: \"%s\"", strerror(errno));
      eprintln("[!] Continuing...");
   }
}

static void outwrite(FILE* handle, const cstr format, ...) {
   va_list args;
   va_start(args, format);
   if (vfprintf(handle, format, args) < 0) {
      panic("[!] Failed to write assembly to output file, reason: \"%s\"", strerror(errno));
   }
   va_end(args);
}

i32 nasm_from_ir(IR* ir, bool asm_dump) {
   mcu_assert(ir != nullptr, "ir can't be null");

   // @Todo: Look into whether or not we should use DEFAULT REL for our NASM generation
   //        - Yuumei-02, 12-07-2026 15:32
   FILE* handle = get_file_handle(asm_dump);
   outwrite(handle,
      "BITS 64\n"
      "global _start\n"
      "section .text\n"
      "\n"
      "_start:\n"
      "   call main\n"
      "   mov eax, 60\n"
      "   xor edi, edi\n"
      "   syscall\n"
      "\n"
      "main:\n");

   foreach (ir->IrInstructions, i) {
      IrInstr* instr = Vector_get(&ir->IrInstructions, i);
      outwrite(handle, "   ; %s\n", IrInstrKind_to_cstr(instr->kind));

      switch (instr->kind) {
         case IIK_PushInt: {
            outwrite(handle, "   push %ld\n", instr->int_value);
         } continue;

         case IIK_Sub:  [[fallthrough]];
         case IIK_Add:  [[fallthrough]];
         case IIK_Idiv: [[fallthrough]];
         case IIK_Imul: [[fallthrough]];
         case IIK_Udiv: [[fallthrough]];
         case IIK_Umul: {
            outwrite(handle,
               "   pop rbx\n"
               "   pop rax\n");
            switch (instr->kind) {
               case IIK_Add:  outwrite(handle, "   add rax, rbx\n");  break;
               case IIK_Sub:  outwrite(handle, "   sub rax, rbx\n");  break;
               case IIK_Imul: [[fallthrough]];
               case IIK_Umul: outwrite(handle, "   imul rax, rbx\n"); break;
               
               case IIK_Idiv: {
                  outwrite(handle,
                     "   cqo\n"
                     "   idiv rbx\n");
               } break;

               case IIK_Udiv: {
                  outwrite(handle,
                     "   xor rdx, rdx\n"
                     "   div rbx\n");
               } break;
               
               default: panic("unreachable");
            }
            outwrite(handle, "   push rax\n");
         } continue;

         case IIK_Puti: {
            outwrite(handle,
               "   ; not yet implemented\n"
               "   pop rax\n");
         } continue;
      }

      panic("unreachable");
   }
      
   outwrite(handle, "   ret\n");
   close_file_handle(handle, asm_dump);

   if (execute_command("nasm -felf64 ./output.asm"))  return 1;
   if (execute_command("ld ./output.o -o ./output"))  return 1;
   if (execute_command("rm ./output.asm ./output.o")) return 1;
   return 0;
}

