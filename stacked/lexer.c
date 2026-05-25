#include <mcu/core.h>
#include <mcu/io.h>

#include <string.h>
#include <errno.h>

#include "lexer.h"

const cstr TokenType_to_cstr(TokenType self) {
   switch (self) {
      case TT_Eof:         return "Eof";
      case TT_Plus:        return "Plus";
      case TT_IntLiteral:  return "IntLiteral";
      case TT_Print:       return "Print";
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

      default: {
         printf("\n");
      }
   }
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

   return self;
}

void Lexer_delete(nullable Lexer* self) {
   if (self == nullptr) return;

   i32 result = fclose(self->file.handle);
   if (result) {
      eprintln("[!] Failed to close file \"%s\", reason: \"%s\"", self->file.path, strerror(errno));
      eprintln("[!] Continuing...");
   }
   
   *self = (Lexer) {0};
}

Token Lexer_next(Lexer* self) {
   unused self;

   Token token = {
      .type = TT_Eof,
   };

   return token;
}
