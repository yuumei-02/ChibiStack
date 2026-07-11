// Copyright (c) 2026 yuumei-02. All Rights Reserved.
// See the LICENSE file for more information.

#include <mcu/core.h>
#include <mcu/io.h>
#include <mcu/memory.h>
#include <mcu/containers.h>

#include <errno.h>
#include <string.h>

#include "lexer.h"

Lexer Lexer_new(cstr file_path) {
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
   self.file_contents[file_size] = '\0';

   isize read = fread(self.file_contents, 1, (usize) file_size, handle);
   if (read != file_size)
      goto read_failure;

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

Token Lexer_next(Lexer* self) {
   mcu_assert(self != nullptr, "self can't be null");

   return (Token) {
      .type = TT_Eof
   };
}

void Token_dump(Token self, Lexer* lexer) {
   unused self;

   mcu_assert(lexer != nullptr, "lexer can't be null");
}

