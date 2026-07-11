// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/memory.h>
#include <mcu/containers.h>

#include <errno.h>
#include <string.h>

#include "lexer.h"

const cstr TokenType_to_cstr(TokenType self) {
   switch (self) {
      case TT_Eof:        return "Eof";
      case TT_Add:        return "Add";
      case TT_Sub:        return "Sub";
      case TT_Idiv:       return "Idiv";
      case TT_Imul:       return "Imul";
      case TT_Udiv:       return "Udiv";
      case TT_Umul:       return "Umul";
      case TT_Word:       return "Word";
      case TT_IntLiteral: return "IntLiteral";
      case TT_Puti:       return "Puti";
   }

   return "Unknown";
}

Lexer Lexer_new(cstr file_path) {
   // @Todo: Convert file_path to an absolute path

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

   return self;

read_failure:
   panic("[!] Failed to read from file \"%s\", reason: \"%s\"", strerror(errno));
}

void Lexer_delete(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   mcu_free(self->file_contents);
   *self = (Lexer) {0};
}

static inline void Lexer_advance(Lexer* self) {
   self->current = self->peek;
   self->peek = self->file_contents[self->z++];
}

Token Lexer_next(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   Token token = {
      .type = TT_Eof,
      .z = self->z
   };
   LexerMode mode = LM_Trim;

   loop {
      Lexer_advance(self);
      if (self->current == '\0')
         break;

      switch (mode) {
         case LM_Trim: {
            token.z = self->z;

            switch (self->current) {
               case '+': token.type = TT_Add; return token;
               case '-': token.type = TT_Sub; return token;

               case '/': {
                  if (self->peek == '/')
                     mode = LM_Comment;
                  else
                     goto default_trim;
               } break;
               
               default: {
                  default_trim:
               }
            }
         } continue;

         case LM_Word: [[fallthrough]];
         case LM_IntLiteral: mcu_todo("not yet implemented");

         case LM_Comment: {
            if (self->current == '\n')
               mode = LM_Trim;
         } continue;
      }

      panic("unreachable");
   }

   return token;
}

void Token_dump(Token self, Lexer* lexer) {
   mcu_assert(lexer != nullptr, "lexer can't be null");

   // @Todo: Show the actual file location
   printf("%s:1:1: info: %s", lexer->file_path, TokenType_to_cstr(self.type));

   switch (self.type) {
      case TT_Word: {
         println(" (%.*s)", (i32) self.str_literal.length, self.str_literal.chars);
      } break;

      case TT_IntLiteral: {
         println(" (%ld)", self.int_literal);
      } break;

      default: printf("\n");
   }
}

