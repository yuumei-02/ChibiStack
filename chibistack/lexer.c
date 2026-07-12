// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/memory.h>
#include <mcu/containers.h>

#include <errno.h>
#include <string.h>

#include "lexer.h"

HashMap_hdr(TokenType)
HashMap_impl(TokenType)

static HashMap(TokenType) G_keywords;
static bool G_keywords_defined = false;

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
   HashMap_put(TokenType)(&G_keywords, "puti",     TT_Puti);
}

Lexer Lexer_new(cstr file_path) {
   // @Todo: Convert file_path to an absolute path
   check_define_keywords();

   Lexer self = {
      .file_path = file_path
   };

   FILE* handle = fopen(file_path, "rb");
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
      c != '\n'
   );
}

Token Lexer_next(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

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

