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
#include "timing.h"

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

static FILE* get_file_handle() {
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

static inline Lexer* get_lexer(IR* ir, u16 lexer_i) {
   return Vector_get(&ir->Lexers, lexer_i);
}

i32 nasm_from_ir(IR* ir, bool asm_dump, double* code_gen_time, double* linker_time) {
   mcu_assert(ir != nullptr, "ir can't be null");
   mcu_assert(code_gen_time != nullptr, "code_gen_time can't be null");
   mcu_assert(linker_time != nullptr, "linker_time can't be null");

   struct timespec timer;
   clock_start(&timer);

   // @Todo: Look into whether or not we should use DEFAULT REL for our NASM generation
   //        - Yuumei-02, 12-07-2026 15:32
   FILE* handle = get_file_handle();
   outwrite(handle,
      "BITS 64\n"
      "global _start\n"
      "section .text\n"
      "\n"
      "_start:\n"
      "   xor rbp, rbp\n"
      "   and rsp, -16\n"
      "   sub rsp, 8\n"
      "   call main\n"
      "   mov eax, 60\n"
      "   xor edi, edi\n"
      "   syscall\n"
      "\n"
      "puti:\n"
      "   push rbp\n"
      "   mov rbp, rsp\n"
      "   sub rsp, 40\n"
      "   mov [rbp-8], rdi\n"
      "   xor eax, eax\n"
      "   mov [rbp-12], eax\n"
      "   mov [rbp-16], eax\n"
      "   cmp qword [rbp-8], 0\n"
      "   jns .check_zero\n"
      "   mov byte [rbp-32], '-'\n"
      "   inc dword [rbp-16]\n"
      "   neg qword [rbp-8]\n"
      "   mov dword [rbp-12], 1\n"
      "   jmp .extract_loop\n"
      ".check_zero:\n"
      "   cmp qword [rbp-8], 0\n"
      "   jne .extract_loop\n"
      "   mov byte [rbp-32], '0'\n"
      "   inc dword [rbp-16]\n"
      "   jmp .prepare_reverse\n"
      ".extract_loop:\n"
      "   cmp qword [rbp-8], 0\n"
      "   jle .prepare_reverse\n"
      "   mov rax, [rbp-8]\n"
      "   xor rdx, rdx\n"
      "   mov rcx, 10\n"
      "   div rcx\n"
      "   mov [rbp-8], rax\n"
      "   mov eax, edx\n"
      "   add al, '0'\n"
      "   mov ecx, [rbp-16]\n"
      "   mov [rbp-32+rcx], al\n"
      "   inc dword [rbp-16]\n"
      "   jmp .extract_loop\n"
      ".prepare_reverse:\n"
      "   mov eax, [rbp-16]\n"
      "   mov [rbp-20], eax\n"
      "   dec eax\n"
      "   mov [rbp-16], eax\n"
      ".reverse_loop:\n"
      "   mov eax, [rbp-16]\n"
      "   cmp eax, [rbp-12]\n"
      "   jle .do_write\n"
      "   mov ecx, [rbp-12]\n"
      "   mov esi, [rbp-16]\n"
      "   mov al, [rbp-32+rcx]\n"
      "   mov bl, [rbp-32+rsi]\n"
      "   mov [rbp-32+rcx], bl\n"
      "   mov [rbp-32+rsi], al\n"
      "   inc dword [rbp-12]\n"
      "   dec dword [rbp-16]\n"
      "   jmp .reverse_loop\n"
      ".do_write:\n"
      "   mov rax, 1\n"
      "   mov rdi, 1\n"
      "   lea rsi, [rbp-32]\n"
      "   mov edx, [rbp-20]\n"
      "   syscall\n"
      "   leave\n"
      "   ret\n"
      "\n");

   u32 stack_element_count = 0;
   
   foreach (ir->IrInstructions, i) {
      IrInstr* instr = Vector_get(&ir->IrInstructions, i);
      outwrite(handle, "   ; %s\n", IrInstrKind_to_cstr(instr->kind));

      switch (instr->kind) {
         case IIK_ProcBegin: {
            char tmp = instr->word.chars[instr->word.length];
            instr->word.chars[instr->word.length] = '\0';
            bool is_main = strcmp(instr->word.chars, "main") == 0;
            instr->word.chars[instr->word.length] = tmp;
         
            if (is_main) {
               outwrite(handle, "main:\n");
            } else {
               outwrite(handle, "%s@%.*s:\n",
                  get_lexer(ir, instr->lexer)->label_path,
                  (i32) instr->word.length, instr->word.chars);
            }

            outwrite(handle,
               "   push rbp\n"
               "   mov rbp, rsp\n");
         } continue;

         case IIK_ProcEnd: {
            outwrite(handle,
               "   mov rsp, rbp\n"
               "   pop rbp\n"
               "   ret\n"
               "\n");
         } continue;

         // @Todo: Add support for passing arguments.
         case IIK_ProcCall: {
            if (stack_element_count > 0 && stack_element_count % 2 == 1) {
               outwrite(handle,
                  "   sub rsp, 8\n"
                  "   call %.*s\n"
                  "   add rsp, 8\n",
                  (i32) instr->word.length, instr->word.chars);
            } else {
               outwrite(handle,
                  "   call %s@%.*s\n",
                  get_lexer(ir, instr->lexer)->label_path,
                  (i32) instr->word.length, instr->word.chars);
            }
         } continue;

         case IIK_Puti: {
            stack_element_count--;
            outwrite(handle, "   pop rdi\n");
            bool misaligned = stack_element_count > 0 && stack_element_count % 2 == 1;
            if (misaligned) outwrite(handle, "   sub rsp, 8\n");
            outwrite(handle, "   call puti\n");
            if (misaligned) outwrite(handle, "   add rsp, 8\n");
         } continue;
      
         case IIK_PushInt: {
            stack_element_count++;
            if (!(instr->int_value >= 0x80000000 && instr->int_value <= 0x7FFFFFFF)) {
               outwrite(handle, "   push %ld\n", instr->int_value);
            } else {
               outwrite(handle,
                  "   mov rax, %ld\n"
                  "   push rax\n",
                  instr->int_value);
            }
         } continue;

         case IIK_PushUint: {
            stack_element_count++;
            if (instr->uint_value < 0x7FFFFFFF) {
               outwrite(handle, "   push %lu\n", instr->uint_value);
            } else {
               outwrite(handle,
                  "   mov rax, %lu\n"
                  "   push rax\n",
                  instr->uint_value);
            }
         } continue;

         case IIK_PushAddr: {
            stack_element_count++;
            outwrite(handle,
               "   mov rax, addr%lu\n"
               "   push rax\n",
               instr->uint_value);
         } continue;

         case IIK_Drop: {
            stack_element_count--;
            outwrite(handle, "   pop rax\n");
         } continue;

         case IIK_Swap: {
            outwrite(handle,
               "   pop rbx\n"
               "   pop rax\n"
               "   push rbx\n"
               "   push rax\n");
         } continue;

         case IIK_Dup: {
            stack_element_count++;
            outwrite(handle,
               "   pop rax\n"
               "   push rax\n"
               "   push rax\n");
         } continue;

         case IIK_Sub:  [[fallthrough]];
         case IIK_Add:  [[fallthrough]];
         case IIK_Idiv: [[fallthrough]];
         case IIK_Udiv: [[fallthrough]];
         case IIK_Mul: {
            stack_element_count--;
            outwrite(handle,
               "   pop rbx\n"
               "   pop rax\n");
            switch (instr->kind) {
               case IIK_Add: outwrite(handle, "   add rax, rbx\n");  break;
               case IIK_Sub: outwrite(handle, "   sub rax, rbx\n");  break;
               case IIK_Mul: outwrite(handle, "   imul rax, rbx\n"); break;
               
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

         case IIK_Syscall0: goto output_syscall;
         case IIK_Syscall1: { stack_element_count -= 1; } goto output_syscall;
         case IIK_Syscall2: { stack_element_count -= 2; } goto output_syscall;
         case IIK_Syscall3: { stack_element_count -= 3; } goto output_syscall;
         case IIK_Syscall4: { stack_element_count -= 4; } goto output_syscall;
         case IIK_Syscall5: { stack_element_count -= 5; } goto output_syscall;
         case IIK_Syscall6: {
            stack_element_count -= 6;
         output_syscall:
            i32 args = ((i32) instr->kind - (i32) IIK_Syscall0) + 1;
            static const cstr syscall_registers[7] = { "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9" };

            for (i32 i = 0; i < args; ++i) {
               outwrite(handle, "   pop %s\n", syscall_registers[i]);
            }

            outwrite(handle,
               "   syscall\n"
               "   push rax\n");
         } continue;

         case IIK_ModuleBegin: continue;
         case IIK_ModuleEnd: continue;
      }

      panic("unreachable");
   }

   outwrite(handle, "section .data\n");

   // @Todo: I don't know if there is a better way to do this but
   //        this method Seems really slow to me.
   //        We should look into whether or not this is a good method.
   //        - Yuumei-02, 12-07-2026 20:33
   foreach (ir->string_literals, i) {
      String* str = Vector_get(&ir->string_literals, i);
      outwrite(handle, "addr%zu: db ", i);
      
      for (usize i = 0; i < str->length; ++i) {
         if (i + 1 < str->length)
            outwrite(handle, "%d,", (i32) str->chars[i]);
         else
            outwrite(handle, "%d\n", (i32) str->chars[i]);
      }
   }
   
   close_file_handle(handle, asm_dump);
   *code_gen_time = clock_end(&timer);

   if (asm_dump) return 0;

   clock_start(&timer);

   if (execute_command("nasm -felf64 ./output.asm"))  goto failure;
   if (execute_command("ld ./output.o -o ./output"))  goto failure;
   if (execute_command("rm ./output.asm ./output.o")) goto failure;

   *linker_time = clock_end(&timer);
   return 0;

failure:
   *linker_time = clock_end(&timer);
   return 1;
}

