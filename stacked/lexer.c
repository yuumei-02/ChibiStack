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
      case TT_StrLiteral: return "StrLiteral";
      case TT_IntLiteral: return "IntLiteral";
      case TT_Drop:       return "Drop";
      case TT_Swap:       return "Swap";
      case TT_Dup:        return "Dup";
      case TT_Syscall1:   return "Syscall1";
      case TT_Syscall2:   return "Syscall2";
      case TT_Syscall3:   return "Syscall3";
      case TT_Syscall4:   return "Syscall4";
      case TT_Syscall5:   return "Syscall5";
      case TT_Syscall6:   return "Syscall6";
      case TT_Puti:       return "Puti";
      case TT_Puts:       return "Puts";
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

      case TT_Identifier: [[fallthrough]];
      case TT_StrLiteral: {
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
   HashMap_put(TokenType)(&G_keywords, "drop",     TT_Drop);
   HashMap_put(TokenType)(&G_keywords, "swap",     TT_Swap);
   HashMap_put(TokenType)(&G_keywords, "dup",      TT_Dup);
   HashMap_put(TokenType)(&G_keywords, "syscall1", TT_Syscall1),
   HashMap_put(TokenType)(&G_keywords, "syscall2", TT_Syscall2);
   HashMap_put(TokenType)(&G_keywords, "syscall3", TT_Syscall3);
   HashMap_put(TokenType)(&G_keywords, "syscall4", TT_Syscall4);
   HashMap_put(TokenType)(&G_keywords, "syscall5", TT_Syscall5);
   HashMap_put(TokenType)(&G_keywords, "syscall6", TT_Syscall6);
   HashMap_put(TokenType)(&G_keywords, "puti",     TT_Puti);
   HashMap_put(TokenType)(&G_keywords, "puts",     TT_Puts);
   
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
               case '/': {
                  if (self->peek == '/') {
                     self->mode = LM_Comment;
                     break;
                  }
                  
                  token.type = TT_Div;
                  return token;
               }

               case '-': {
                  if (self->peek >= '0' && self->peek <= '9') {
                     int_negative = true;
                     self->mode = LM_Integer;
                     break;
                  }

                  token.type = TT_Sub;
                  return token;
               }

               case '"': {
                  self->mode = LM_String;
               } break;
               
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

            if (self->peek == ' '|| self->peek == '\n' || self->peek == '\t' || self->peek == EOF) {
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

         case LM_String: {
            if (self->current == '\\') {
               switch (self->peek) {
                  case 'n': String_append(&self->accumulated, '\n'); break;
                  case 't': String_append(&self->accumulated, '\t'); break;
                  case '0': String_append(&self->accumulated, '\0'); break;
                  default: {
                     eprintln("[!] %s:%zu:%zu:%d Unknown escape sequence \"\\%c\"", self->file.path, self->y, self->x, 2, self->peek);
                  } break;
               }
               Lexer_advance(self);
            } else {
               String_append(&self->accumulated, self->current);
            }

            if (self->peek == '"' || self->peek == EOF) {
               Lexer_advance(self);
               token.type = TT_StrLiteral;
               token.length = self->accumulated.length + 2;
               token.str_literal = String_clone(self->accumulated);
               return token;
            }
         } continue;

         case LM_Comment: {
            if (self->peek == '\n' || self->peek == EOF) {
               self->mode = LM_Trim;
            }
         } continue;
      }

      panic("unreachable");
   }

   return token;
}
