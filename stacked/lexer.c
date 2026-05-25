#include <mcu/core.h>
#include <mcu/containers.h>
#include <mcu/memory.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>

#include "lexer.h"

HashMap_hdr(TokenType)
HashMap_impl(TokenType)

static HashMap(TokenType) G_keywords;
static bool G_keywords_allocated = false;

const cstr TokenType_to_cstr(TokenType self) {
   switch (self) {
      case TT_Eof:        return "Eof";
      case TT_Add:        return "Add";
      case TT_Sub:        return "Sub";
      case TT_Mul:        return "Mul";
      case TT_Div:        return "Div";
      case TT_Identifier: return "Identifier";
      case TT_IntLiteral: return "IntLiteral";
      case TT_Puti:       return "Puti";
   }

   return "Unknown";
}

void Token_display(const cstr path, Token self) {
   mcu_assert(path != nullptr, "path can't be null");
   
   printf("%s:%zu:%zu:%zu: %s", path, self.y, self.x, self.length, TokenType_to_cstr(self.type));

   switch (self.type) {
      case TT_IntLiteral: {
         println(" (%ld)", self.int_literal);
      } break;

      case TT_Identifier: {
         println(" (%s)", self.str_literal.chars);
      } break;

      default: {
         printf("\n");
      }
   }
}

void Token_free(Token self) {
   if (self.type == TT_Identifier) {
      String_free(&self.str_literal);
   }
}

void check_allocate_keywords() {
   if (G_keywords_allocated) return;

   G_keywords = HashMap_new(TokenType)();
   HashMap_put(TokenType)(&G_keywords, "puti", TT_Puti);
   
   G_keywords_allocated = true;
}

Lexer Lexer_new(cstr filepath) {
   mcu_assert(filepath != nullptr, "filepath can't be null");

   Lexer self = {
      .y = 1,
      .mode = LM_Trim,
      .accumulated = String_new(),
      .file = {
         .path = filepath,
         .handle = fopen(filepath, "rb")
      }
   };

   if (self.file.handle == nullptr) {
      panic("[!] Failed to open file \"%s\", reason: \"%s\"", filepath, strerror(errno));
   }

   self.peek = fgetc(self.file.handle);
   if (self.peek == EOF && ferror(self.file.handle)) {
      panic("[!] Failed to read from file \"%s\", reason: \"%s\"", filepath, strerror(errno));
   }

   check_allocate_keywords();
   return self;
}

void Lexer_delete(nullable Lexer* self) {
   if (self == nullptr) return;

   String_free(&self->accumulated);

   i32 result = fclose(self->file.handle);
   if (result) {
      eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", self->file.path, strerror(errno));
      eprintln("[!] Continuing...");
   }
   
   *self = (Lexer) {0};
}

static void Lexer_advance(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   if (self->current == '\n') {
      self->x = 1;
      self->y += 1;
   } else {
      self->x += 1;
   }

   self->current = self->peek;
   self->peek = fgetc(self->file.handle);
   if (self->peek == EOF && ferror(self->file.handle)) {
      panic("[!] Failed to read from file \"%s\", reason: \"%s\"", self->file.path, strerror(errno));
   }
}

Token Lexer_next(Lexer* self) {
   Token token = {
      .x = self->x,
      .y = self->y,
      .length = 1,
      .type = TT_Eof,
   };

   self->mode = LM_Trim;
   String_clear(&self->accumulated);

   bool int_negative = false;

   loop {
      Lexer_advance(self);
      if (self->current == EOF) break;

   reparse_char:
      switch (self->mode) {
         case LM_Trim: {
            token.x = self->x;
            token.y = self->y;

            switch (self->current) {
               case '+': token.type = TT_Add; return token;
               case '*': token.type = TT_Mul; return token;
               case '/': token.type = TT_Div; return token;

               case '-': {
                  if (self->peek >= '0' && self->peek <= '9') {
                     int_negative = true;
                     self->mode = LM_Integer;
                     break;
                  }

                  token.type = TT_Sub;
                  return token;
               }
               
               case '\t': break;
               case '\n': break;
               case ' ':  break;
               
               default: {
                  if (self->current >= '0' && self->current <= '9') {
                     int_negative = false;
                     self->mode = LM_Integer;
                  } else {
                     self->mode = LM_Normal;
                  }

                  goto reparse_char;
               }
            }
         } continue;
         
         case LM_Normal: {
            String_append(&self->accumulated, self->current);

            if (self->peek == EOF || self->peek == ' '|| self->peek == '\n' || self->peek == '\t') {
               TokenType* keyword = HashMap_get(TokenType)(&G_keywords, self->accumulated.chars);
               if (keyword == nullptr) {
                  token.type = TT_Identifier;
                  token.str_literal = String_clone(self->accumulated);
               } else {
                  token.type = *keyword;
               }
               
               token.length = self->accumulated.length;
               return token;
            }
         } continue;
         
         case LM_Integer: {
            token.int_literal *= 10;
            token.int_literal += self->current - '0';
            token.length += 1;

            if (!(self->peek >= '0' && self->peek <= '9')) {
               token.type = TT_IntLiteral;
               if (int_negative)
                  token.int_literal = -token.int_literal;
               else
                  token.length -= 1;
               return token;
            }
         } continue;
      }

      panic("unreachable");
   }

   return token;
}
