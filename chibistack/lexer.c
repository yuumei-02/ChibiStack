// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#define _XOPEN_SOURCE 500
#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/memory.h>
#include <mcu/containers.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

#include "lexer.h"
#include "timing.h"

HashMap_hdr(TokenType)
HashMap_impl(TokenType)

static HashMap(TokenType) G_keywords;
static bool G_keywords_defined = false;

static u32 tokens_parsed = 0;

const cstr TokenType_to_cstr(TokenType self) {
   switch (self) {
      case TT_Eof:        return "Eof";
      case TT_Drop:       return "Drop";
      case TT_Swap:       return "Swap";
      case TT_Dup:        return "Dup";
      case TT_Add:        return "Add";
      case TT_Sub:        return "Sub";
      case TT_Idiv:       return "Idiv";
      case TT_Udiv:       return "Udiv";
      case TT_Mul:        return "Mul";
      case TT_Dot:        return "Dot";
      case TT_Word:       return "Word";
      case TT_IntLiteral: return "IntLiteral";
      case TT_StrLiteral: return "StrLiteral";
      case TT_Syscall0:   return "Syscall0";
      case TT_Syscall1:   return "Syscall1";
      case TT_Syscall2:   return "Syscall2";
      case TT_Syscall3:   return "Syscall3";
      case TT_Syscall4:   return "Syscall4";
      case TT_Syscall5:   return "Syscall5";
      case TT_Syscall6:   return "Syscall6";
      case TT_Proc:       return "Proc";
      case TT_Begin:      return "Begin";
      case TT_End:        return "End";
      case TT_Include:    return "#include";
      case TT_Puti:       return "Puti";
   }

   return "Unknown";
}

static void check_define_keywords() {
   if (G_keywords_defined) return;

   G_keywords_defined = true;
   G_keywords = HashMap_new(TokenType)();

   HashMap_put(TokenType)(&G_keywords, "drop",     TT_Drop);
   HashMap_put(TokenType)(&G_keywords, "swap",     TT_Swap);
   HashMap_put(TokenType)(&G_keywords, "dup",      TT_Dup);
   HashMap_put(TokenType)(&G_keywords, "idiv",     TT_Idiv);
   HashMap_put(TokenType)(&G_keywords, "udiv",     TT_Udiv);
   HashMap_put(TokenType)(&G_keywords, "syscall0", TT_Syscall0);
   HashMap_put(TokenType)(&G_keywords, "syscall1", TT_Syscall1);
   HashMap_put(TokenType)(&G_keywords, "syscall2", TT_Syscall2);
   HashMap_put(TokenType)(&G_keywords, "syscall3", TT_Syscall3);
   HashMap_put(TokenType)(&G_keywords, "syscall4", TT_Syscall4);
   HashMap_put(TokenType)(&G_keywords, "syscall5", TT_Syscall5);
   HashMap_put(TokenType)(&G_keywords, "syscall6", TT_Syscall6);
   HashMap_put(TokenType)(&G_keywords, "proc",     TT_Proc);
   HashMap_put(TokenType)(&G_keywords, "begin",    TT_Begin);
   HashMap_put(TokenType)(&G_keywords, "end",      TT_End);
   HashMap_put(TokenType)(&G_keywords, "#include",  TT_Include);
   HashMap_put(TokenType)(&G_keywords, "puti",     TT_Puti);
}

String path_strip_file(cstr path) {
   mcu_assert(path != nullptr, "path can't be null");

   String new_path = String_from(path);
   for (isize i = new_path.length - 1; i >= 0; --i) {
      if (new_path.chars[i] == '/') break;
      String_pop(&new_path);
   }

   return new_path;
}

// @Todo: Add actual file IO error reporting.
Lexer Lexer_new(cstr file_path, nullable cstr relative_to) {
   check_define_keywords();

   char cwd[PATH_MAX] = {0};

   if (relative_to != nullptr) {
      if (getcwd((cstr) &cwd, PATH_MAX) == nullptr) {
         panic("[!] Failed to get the current working directory, reason: \"%s\"", strerror(errno));
      }

      String relative = path_strip_file(relative_to);
      if (chdir(relative.chars)) {
         panic("[!] Failed to change the current working directory, reason: \"%s\"", strerror(errno));
      }
      String_free(&relative);
   }

   Lexer self = {
      .file_path = file_path,
      .full_path = realpath(file_path, nullptr)
   };

   if (self.full_path == nullptr) {
      panic("[!] Failed to expand file path \"%s\", reason: \"%s\"", file_path, strerror(errno));
   }

   if (relative_to != nullptr) {
      if (chdir(cwd) < 0) {
         panic("[!] Failed to change the current working directory, reason: \"%s\"", strerror(errno));
      }
   }

   FILE* handle = fopen(self.full_path, "rb");
   if (handle == nullptr)
      panic("[!] Failed to open file \"%s\", reason: \"%s\"", strerror(errno));

   if (fseek(handle, 0, SEEK_END))
      goto read_failure;

   isize file_size = ftell(handle);
   if (file_size < 0)
      goto read_failure;

   // @Todo: I don't know whether or not rewind can fail.
   //        I couldn't find any clear info about it in the man page
   //        We may want to look into it at some point.
   //        - Yuumei-02, 12-07-2026 00:29
   rewind(handle);

   self.file_contents = mcu_malloc((usize) file_size + 1);

   isize read = fread(self.file_contents, 1, (usize) file_size, handle);
   if (read != file_size)
      goto read_failure;

   self.file_contents[file_size] = '\0';
   self.peek = self.file_contents[self.z++];

   if (fclose(handle)) {
      eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", file_path, strerror(errno));
      eprintln("[!] Continuing...");
   }

   self.new_line_indices = Vector_new(sizeof(u32));
   return self;

read_failure:
   panic("[!] Failed to read from file \"%s\", reason: \"%s\"", strerror(errno));
}

void Lexer_delete(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   Vector_free(&self->new_line_indices);
   mcu_free(self->file_contents);
   mcu_free(self->full_path);
   *self = (Lexer) {0};
}

static inline void Lexer_advance(Lexer* self) {
   if (self->peek == '\n') {
      Vector_push_create(&self->new_line_indices, ((u32) self->z - 1));
   }

   self->current = self->peek;
   self->peek = self->file_contents[self->z++];
}

static bool word_allowed(char c) {
   return (
      c != '+' &&
      c != '-' &&
      c != ' ' &&
      c != '\t' &&
      c != '\n' &&
      c != '.'
   );
}

[[gnu::always_inline]]
static inline Token Lexer_next_impl(Lexer* self) { 
   Token token = {
      .type = TT_Eof,
      .z = self->z
   };
   LexerMode mode = LM_Trim;
   bool num_is_negative = false;

   loop {
      Lexer_advance(self);
      if (self->current == '\0')
         break;

   reparse_char:
      switch (mode) {
         case LM_Trim: {
            token.z = self->z;

            // @Note: Don't forget to update word_allowed when adding new non word characters!
            switch (self->current) {
               case '+': token.type = TT_Add; return token;
               case '*': token.type = TT_Mul; return token;
               case '.': token.type = TT_Dot; return token;

               case '-': {
                  if (!(self->peek >= '0' && self->peek <= '9')) {
                     token.type = TT_Sub;
                     return token;
                  } else {
                     num_is_negative = true;
                     mode = LM_IntLiteral;
                  }
               } break;

               case '/': {
                  if (self->peek == '/')
                     mode = LM_Comment;
                  else
                     goto default_trim;
               } break;

               case '"': {
                  token.str_literal = String_new();
                  mode = LM_StrLiteral;
               } break;

               case ' ': break;
               case '\t': break;
               case '\n': break;
               
               default: {
               default_trim:
                  if (self->current >= '0' && self->current <= '9') {
                     num_is_negative = false;
                     mode = LM_IntLiteral;
                  } else {
                     token.str_view = (StringView) {
                        .chars = self->file_contents + (self->z - 2),
                        .length = 0
                     };
                     mode = LM_Word;
                  }
                  goto reparse_char;
               }
            }
         } continue;

         case LM_Word: {
            token.str_view.length++;

            if (!word_allowed(self->peek)) {
               char tmp = token.str_view.chars[token.str_view.length];
               token.str_view.chars[token.str_view.length] = '\0';
               TokenType* keyword = HashMap_get(TokenType)(&G_keywords, token.str_view.chars);
               token.str_view.chars[token.str_view.length] = tmp;

               if (keyword == nullptr)
                  token.type = TT_Word;
               else
                  token.type = *keyword;
               return token;
            }
         } continue;

         case LM_IntLiteral: {
            token.int_literal *= 10;
            token.int_literal += self->current - '0';

            if (!(self->peek >= '0' && self->peek <= '9')) {
               token.type = TT_IntLiteral;
               if (num_is_negative)
                  token.int_literal = -token.int_literal;
               
               return token;
            }
         } continue;

         case LM_StrLiteral: {
            switch (self->current) {
               case '\\': {
                  Lexer_advance(self);

                  // @Todo: Report invalid escape sequences
                  switch (self->current) {
                     case 'n': String_append(&token.str_literal, '\n'); break;
                     case '\0': goto StrLiteralEofCase;
                     default: {
                        String_append(&token.str_literal, '\\');
                        String_append(&token.str_literal, self->current);
                     }
                  }
               } break;

               default: String_append(&token.str_literal, self->current);
            }

            // In order to prevent a memory leak for when the String never terminates due to EOF
            if (self->peek == EOF) {
               String_free(&token.str_literal);
            }

            if (self->peek == '"') {
               Lexer_advance(self);
               token.type = TT_StrLiteral;
               return token;
            }

            continue;
         StrLiteralEofCase:
            String_free(&token.str_literal);
            return token;
         } continue;

         case LM_Comment: {
            if (self->current == '\n')
               mode = LM_Trim;
         } continue;
      }

      panic("unreachable");
   }

   return token;
}

Token Lexer_next(Lexer* self, double* lexer_time) {
   mcu_assert(self != nullptr, "self can't be null");
   mcu_assert(lexer_time != nullptr, "lexer_time can't be null");

   struct timespec timer;

   tokens_parsed += 1;
   clock_start(&timer);
   Token token = Lexer_next_impl(self);
   *lexer_time += clock_end(&timer);

   return token;
}

u32 get_tokens_parsed() {
   return tokens_parsed;
}

Loc Lexer_loc_from_offset(Lexer* self, u32 offset) {
   mcu_assert(self != nullptr, "self can't be null");

   Loc loc = { .x = offset, .y = 1 };
   u32 subtract = 0;
   
   foreach (self->new_line_indices, i) {
      u32 new_line = *(u32*) Vector_get(&self->new_line_indices, i);

      if (new_line < offset) {
         loc.y++;
         subtract = new_line + 2;
      } else {
         break;
      }
   }

   loc.x -= subtract;
   return loc;
}

void Token_dump(Token self, Lexer* lexer) {
   mcu_assert(lexer != nullptr, "lexer can't be null");

   Loc loc = Lexer_loc_from_offset(lexer, self.z);
   printf("%s:%u:%u: info: %s", lexer->file_path, loc.y, loc.x, TokenType_to_cstr(self.type));

   switch (self.type) {
      case TT_Word: {
         println(" (%.*s)", (i32) self.str_view.length, self.str_view.chars);
      } break;

      case TT_IntLiteral: {
         println(" (%ld)", self.int_literal);
      } break;

      case TT_StrLiteral: {
         println(" (%s)", self.str_literal.chars);
      } break;

      default: printf("\n");
   }
}

